#include <cstdio>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
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

#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "optimiser.hpp"

namespace Kepler::Compiler {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;

    static std::map<std::string, llvm::Value*> named_values;

    static llvm::TargetMachine* target_machine;

    const static bool write_file(const char* outname);
    const static bool initialise();

    namespace Internal {

        llvm::LLVMContext& get_context() {
            return *context;
        }

        llvm::IRBuilder<>& get_builder() {
            return *builder;
        }

        llvm::Module& get_module() {
            return *module;
        }

        std::map<std::string, llvm::Value*>& get_named_values() {
            return named_values;
        }

    }


    const bool compile_file(const char* filename, const char* outname) {
        if (!Lexer::initialise(filename)) {
            return false;
        }
        if(!initialise()) {
            return false;
        }

        while (true) {
            switch (Parser::get_current_token()) {
                case Lexer::Token_EndOfFile:
                    Lexer::cleanup();
                    return write_file(outname);
                case Lexer::Token_Function:
                    Parser::handle_function();
                    break;
                case Lexer::Token_Extern:
                    Parser::handle_extern();
                    break;
                default:
                    Parser::handle_top_level_expression();
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
            llvm::errs() << error;
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
        std::error_code ec;
        llvm::raw_fd_ostream out(outname, ec, llvm::sys::fs::OF_None);

        if (ec) {
            llvm::errs() << "Error: Failed to open output file: " << ec.message();
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        if (target_machine->addPassesToEmitFile(pass_manager, out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            llvm::errs() << "Error: Target machine can't emit file of type 'Object File'";
            return false;
        }

        pass_manager.run(*module);
        out.flush();
        llvm::outs() << "Wrote " << outname << "\n";

        return true;
    }
}
