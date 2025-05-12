#pragma once

#include "ast/prototype.hpp"
#include "ast/variable_data.hpp"
#include "file.hpp"
#include "target_type_stack.hpp"
#include "type.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace Kepler::Compiler {

    llvm::LLVMContext& get_context();
    llvm::IRBuilder<>& get_builder();
    llvm::Module& get_module();
    std::map<std::string, std::pair<TypeToken, llvm::AllocaInst*>>& get_local_variables();
    std::map<std::string, std::shared_ptr<AST::Prototype>>& get_prototypes();
    std::unique_ptr<File>& get_file();
    TargetTypeStack& get_target_type_stack();
    void set_function_return_type(TypeToken type);
    TypeToken get_function_return_type();

    std::optional<AST::VariableData> get_local_variable(const std::string& name);
    bool compile_file(const char* filename, const char* outname);

}
