#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "runtime.hpp"
#include "../ast/prototype.hpp"
#include "../compiler.hpp"
#include "../function_registry/function_registry.hpp"
#include "../log.hpp"
#include "../types/type.hpp"

extern unsigned char __kepler_runtime_bc[];
extern unsigned int __kepler_runtime_bc_len;

namespace Kepler::Runtime {

    static void register_runtime_functions() {
        // __kepler_runtime_init
        std::shared_ptr<AST::Prototype> runtime_init_prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "__kepler_runtime_init", std::vector<AST::ParameterData>());
        FunctionRegistry::register_prototype(runtime_init_prototype);

        // __kepler_string_length
        std::vector<AST::ParameterData> string_length_args = {
            { Type::TypeToken::String, "a" },
        };
        std::shared_ptr<AST::Prototype> string_length_prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Int64, "__kepler_string_length", std::move(string_length_args));
        FunctionRegistry::register_prototype(string_length_prototype);

        // __kepler_string_concat
        std::vector<AST::ParameterData> string_concat_args = {
            { Type::TypeToken::String, "a" },
            { Type::TypeToken::String, "b" }
        };
        std::shared_ptr<AST::Prototype> string_concat_prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_string_concat", std::move(string_concat_args));
        FunctionRegistry::register_prototype(string_concat_prototype);

        // print
        std::vector<AST::ParameterData> print_args = {
            { Type::TypeToken::String, "message" },
        };
        std::shared_ptr<AST::Prototype> print_prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "print", std::move(print_args));
        FunctionRegistry::register_prototype(print_prototype);

        // pause
        std::shared_ptr<AST::Prototype> pause_prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "pause", std::vector<AST::ParameterData>());
        FunctionRegistry::register_prototype(pause_prototype);
    }

    std::unique_ptr<llvm::Module> create() {
        llvm::StringRef runtime_buffer(reinterpret_cast<const char*>(__kepler_runtime_bc), __kepler_runtime_bc_len);
        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> memory_buffer = llvm::MemoryBuffer::getMemBuffer(runtime_buffer, "runtime_buffer", false);
        if (!memory_buffer) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to read runtime bytecode into memory buffer");
            return nullptr;
        }

        llvm::Expected<std::unique_ptr<llvm::Module>> runtime_module = llvm::parseBitcodeFile(*memory_buffer.get(), Compiler::get_context());
        if (!runtime_module) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to parse runtime bytecode into llvm module");
            return nullptr;
        }

        register_runtime_functions();

        return std::move(*runtime_module);
    }

}
