#pragma once

#include "ast/prototype.hpp"
#include "ast/variable_data.hpp"
#include "file.hpp"
#include "type.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace Kepler::Compiler {

    namespace TargetTypeStack {

        void push(Type::TypeToken type);
        void pop();
        Type::TypeToken top();

    }

    llvm::LLVMContext& get_context();
    llvm::IRBuilder<>& get_builder();
    llvm::Module& get_module();
    std::map<std::string, std::pair<Type::TypeToken, llvm::AllocaInst*>>& get_local_variables();
    std::map<std::string, std::shared_ptr<AST::Prototype>>& get_prototypes();
    std::unique_ptr<File>& get_file();
    void set_function_return_type(Type::TypeToken type);
    Type::TypeToken get_function_return_type();

    std::optional<AST::VariableData> get_local_variable(const std::string& name);
    bool compile_file(const char* filename, const char* outname);

}
