#pragma once

#include "file.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <memory>

namespace Kepler::Compiler {

    llvm::LLVMContext& get_context();
    llvm::IRBuilder<>& get_builder();
    llvm::Module& get_module();
    std::unique_ptr<File>& get_file();

    bool compile_file(const std::string& filename, const std::string& outname);

}
