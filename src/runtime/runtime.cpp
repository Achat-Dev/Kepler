// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "runtime/runtime.hpp"

#include "ast/prototype.hpp"
#include "compiler.hpp"
#include "function_registry/function_registry.hpp"
#include "log.hpp"
#include "types/type_token.hpp"

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

extern unsigned char __kepler_runtime_bc[];
extern unsigned int __kepler_runtime_bc_len;

namespace Kepler::Runtime {

    static void register_error() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::String, "message" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "error", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_print() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::String, "message" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "print", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_pause() {
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "pause", std::vector<AST::ParameterData>());
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_runtime_init() {
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Void, "__kepler_runtime_init", std::vector<AST::ParameterData>());
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_string_length() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::String, "a" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::Int64, "__kepler_string_length", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_string_concat() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::String, "a" },
            { Type::TypeToken::String, "b" }
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_string_concat", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_string_compare() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::String, "a" },
            { Type::TypeToken::String, "b" }
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_string_compare", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_bool_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Bool, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_bool_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_i8_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Int8, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_i8_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_i16_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Int16, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_i16_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_i32_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Int32, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_i32_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_i64_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Int64, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_i64_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_f32_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Float32, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_f32_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_kepler_f64_to_string() {
        std::vector<AST::ParameterData> args = {
            { Type::TypeToken::Float64, "value" },
        };
        std::shared_ptr<AST::Prototype> prototype = std::make_shared<AST::Prototype>(Type::TypeToken::String, "__kepler_f64_to_string", std::move(args));
        FunctionRegistry::register_prototype(prototype);
    }

    static void register_runtime_functions() {
        register_print();
        register_pause();
        register_kepler_runtime_init();
        register_error();
        register_kepler_string_length();
        register_kepler_string_concat();
        register_kepler_string_compare();
        register_kepler_bool_to_string();
        register_kepler_i8_to_string();
        register_kepler_i16_to_string();
        register_kepler_i32_to_string();
        register_kepler_i64_to_string();
        register_kepler_f32_to_string();
        register_kepler_f64_to_string();
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
