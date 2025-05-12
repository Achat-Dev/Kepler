#include <cassert>
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
#include <optional>
#include <string>
#include <utility>

#include "ast/prototype.hpp"
#include "ast/variable_data.hpp"
#include "file.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "optimiser.hpp"
#include "target_type_stack.hpp"
#include "type.hpp"

namespace Kepler::Compiler {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;

    static std::map<std::string, std::pair<TypeToken, llvm::AllocaInst*>> local_variables;
    static std::map<std::string, std::shared_ptr<AST::Prototype>> prototypes;
    static TargetTypeStack target_type_stack;

    static llvm::TargetMachine* target_machine;

    static std::unique_ptr<File> file;
    static TypeToken function_return_type = TypeToken::None;

    static bool write_file(const char* outname);
    static bool initialise();

    llvm::LLVMContext& get_context() {
        return *context;
    }

    llvm::IRBuilder<>& get_builder() {
        return *builder;
    }

    llvm::Module& get_module() {
        return *module;
    }

    std::map<std::string, std::pair<TypeToken, llvm::AllocaInst*>>& get_local_variables() {
        return local_variables;
    }

    std::map<std::string, std::shared_ptr<AST::Prototype>>& get_prototypes() {
        return prototypes;
    }

    std::unique_ptr<File>& get_file() {
        return file;
    }

    TargetTypeStack& get_target_type_stack() {
        return target_type_stack;
    }

    void set_function_return_type(TypeToken type) {
        assert((function_return_type == TypeToken::None || type == TypeToken::None) && "[ Assertion ]: trying to set function return type when it is already set");
        function_return_type = type;
    }

    TypeToken get_function_return_type() {
        return function_return_type;
    }

    std::optional<AST::VariableData> get_local_variable(const std::string& name) {
        if (local_variables.find(name) != local_variables.end()) {
            std::pair<TypeToken, llvm::AllocaInst*>& variable = local_variables[name];
            return AST::VariableData{ variable.first, variable.second };
        }
        return std::nullopt;
    }

    bool compile_file(const char* filename, const char* outname) {
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
                case Lexer::Token_Extern:
                    if (!Parser::handle_extern()) {
                        return false;
                    }
                    break;
                case Lexer::Token_DataType:
                    if (!Parser::handle_data_type()) {
                        return false;
                    }
                    break;
                default:
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid token on top level (expected 'extern' or data type)");
                    return false;
            }
        }
    }

    static bool initialise() {
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

    static bool write_file(const char* outname) {
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
