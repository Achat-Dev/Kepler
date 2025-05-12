#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <memory>
#include <string>

#include "ast/prototype.hpp"
#include "types/type.hpp"
#include "file.hpp"

namespace Kepler::Compiler {

    llvm::LLVMContext& get_context();
    llvm::IRBuilder<>& get_builder();
    llvm::Module& get_module();
    std::map<std::string, std::shared_ptr<AST::Prototype>>& get_prototypes();
    std::unique_ptr<File>& get_file();
    void set_function_return_type(Type::TypeToken type);
    Type::TypeToken get_function_return_type();

    bool compile_file(const char* filename, const char* outname);

}
