#include <cstdio>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <map>
#include <memory>
#include <string>

#include "file.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "optimiser.hpp"

namespace Kepler::Compiler {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;

    static std::map<std::string, llvm::AllocaInst*> named_values;

    static llvm::TargetMachine* target_machine;

    static std::unique_ptr<File> file;

    const static bool write_file(const char* outname);
    const static bool initialise();

    llvm::LLVMContext& get_context() {
        return *context;
    }

    llvm::IRBuilder<>& get_builder() {
        return *builder;
    }

    llvm::Module& get_module() {
        return *module;
    }

    std::map<std::string, llvm::AllocaInst*>& get_named_values() {
        return named_values;
    }

    std::unique_ptr<File>& get_file() {
        return file;
    }

    const bool compile_file(const char* filename, const char* outname) {
        if(!initialise()) {
            return false;
        }

        if (!(file = File::create(filename))) {
            return false;
        }

        // Read first token to kick things off
        Parser::read_next_token();

        while (true) {
            switch (Parser::get_current_token()) {
                case Lexer::Token_EndOfFile:
                    file->close();
                    return write_file(outname);
                case Lexer::Token_Function:
                    if (!Parser::handle_function()) {
                        return false;
                    }
                    break;
                case Lexer::Token_Extern:
                    if (!Parser::handle_extern()) {
                        return false;
                    }
                    break;
                default:
                    if (!Parser::handle_top_level_expression()) {
                        return false;
                    }
                    break;
            }
        }
    }

    const static bool initialise() {
        context = std::make_unique<llvm::LLVMContext>();
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
        module = std::make_unique<llvm::Module>("Kepler", *context);

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

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

        Optimiser::initialise();

        return true;
    }

    const static bool write_file(const char* outname) {
        log("Writing file '", outname, '\'');

        std::error_code ec;
        llvm::raw_fd_ostream out(outname, ec, llvm::sys::fs::OF_None);

        if (ec) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": failed to open output file: ", ec.message());
            return false;
        }

        if (llvm::verifyModule(*module, &llvm::errs())) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": compiled code is faulty");
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        if (target_machine->addPassesToEmitFile(pass_manager, out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            log(LogStyle::ERROR, "[ Writing error ]", LogStyle::DEFAULT, ": target machine can't emit file of type 'Object File'");
            return false;
        }

        pass_manager.run(*module);
        out.flush();
        log("Successfully wrote '", outname, '\'');

        return true;
    }
}
