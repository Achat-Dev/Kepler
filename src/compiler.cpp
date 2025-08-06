#include "compiler.hpp"

#include "arguments.hpp"
#include "ast/call_expression.hpp"
#include "ast/expression.hpp"
#include "file.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "parser.hpp"
#include "optimiser.hpp"
#include "runtime/runtime.hpp"
#include "types/tmap_type.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace Kepler::Compiler {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;

    static llvm::TargetMachine* target_machine;

    static std::unique_ptr<File> file;

    static bool initialise() {
        context = std::make_unique<llvm::LLVMContext>();
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
        module = std::make_unique<llvm::Module>("Kepler", *context);

        // Initialise llvm stuff
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        // Create target triple to output object code
        std::string target_triple_string = llvm::sys::getDefaultTargetTriple();
        llvm::Triple target_triple(target_triple_string);
        module->setTargetTriple(target_triple);

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple_string, error);

        if (!target) {
            log(LogStyle::ERROR, "[ Initialisation error ]", LogStyle::DEFAULT, ": ", error);
            return false;
        }

        std::string cpu = "generic";
        std::string features = "";
        llvm::TargetOptions target_options;
        target_machine = target->createTargetMachine(target_triple, cpu, features, target_options, llvm::Reloc::PIC_);

        module->setDataLayout(target_machine->createDataLayout());

        // Initalise internal stuff
        Type::TMapType::create_type();

        // Read the runtime bytecode
        std::unique_ptr<llvm::Module> runtime_module = Runtime::create();
        if (!runtime_module) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to create the runtime");
            return false;
        }

        // Link the runtime
        llvm::Linker linker(*module);
        if (linker.linkInModule(std::move(runtime_module))) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to link the runtime");
            return false;
        }

        Optimiser::initialise();

        return true;
    }

    static bool write_object_file(const std::string& output_name) {
        log("Writing file '", output_name, '\'');

        std::error_code ec;
        llvm::raw_fd_ostream out(output_name, ec, llvm::sys::fs::OF_None);

        if (ec) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": failed to open output file: ", ec.message());
            return false;
        }

        if (llvm::verifyModule(*module, &llvm::errs())) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": compiled object code is faulty");
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        if (target_machine->addPassesToEmitFile(pass_manager, out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": target machine can't emit file of type 'Object File'");
            return false;
        }

        pass_manager.run(*module);
        out.flush();
        log("Successfully wrote '", output_name, '\'');

        return true;
    }

    static bool compile_executable(const std::string& outname) {
        // Create call to __kepler_runtime_init at the beginning of the main function
        llvm::Function* main_function = module->getFunction("main");
        llvm::BasicBlock& main_entry_block = main_function->getEntryBlock();
        builder->SetInsertPoint(main_entry_block.begin());
        AST::CallExpression gc_init_call_expression = AST::CallExpression("__kepler_runtime_init", std::vector<std::unique_ptr<AST::Expression>>());
        gc_init_call_expression.codegen();

        // Compile module to object file
        std::string object_outname = outname + ".o";
        if (!write_object_file(object_outname)) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": failed to write object file");
            return false;
        }

        // Call clang to compile the final executable
        log("Compiling object file into executable");
        std::stringstream command;
        command << "clang++ " << object_outname;
        const std::vector<std::string>& additional_files = Arguments::get_additional_files();
        for (const std::string& additional_file : additional_files) {
            log(additional_file);
            if (!std::filesystem::exists(additional_file)) {
                log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": addtional file '", additional_file, "' doesn't exist");
                return false;
            }
            command << " " << additional_file;
        }
        command << " -lgc -o " << outname;
        const int clang_result = std::system(command.str().c_str());

        if (clang_result == 0) {
            log("Successfully wrote '", outname, "\'\nHoly shit it actually worked o.o");
            return true;
        }
        else {
            log(LogStyle::ERROR, "[ Writing Error ]", LogStyle::DEFAULT, ": failed to compile final executable");
            return false;
        }
    }

    llvm::LLVMContext& get_context() {
        return *context;
    }

    llvm::IRBuilder<>& get_builder() {
        return *builder;
    }

    llvm::Module& get_module() {
        return *module;
    }

    std::unique_ptr<File>& get_file() {
        return file;
    }

    bool compile_file() {
        if(!initialise()) {
            return false;
        }

        if (!(file = File::create(Arguments::get_input_file()))) {
            log(LogStyle::ERROR, "[ Reading error ]", LogStyle::DEFAULT, ": input file '", Arguments::get_input_file(), "' doesn't exist");
            return false;
        }

        // Read first token to kick things off
        Parser::read_next_token();

        while (true) {
            switch (Parser::get_current_token()) {
                case Lexer::Token::EndOfFile:
                    file->close();
                    return compile_executable(Arguments::get_output_file());
                case Lexer::Token::Extern:
                    if (!Parser::handle_top_level_extern()) {
                        return false;
                    }
                    break;
                case Lexer::Token::DataType:
                    if (!Parser::handle_top_level_data_type()) {
                        return false;
                    }
                    break;
                default:
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid token '", Parser::get_current_token(), "' on top level, expected 'extern' or data type");
                    return false;
            }
        }
    }
}
