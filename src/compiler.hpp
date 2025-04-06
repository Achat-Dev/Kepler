#pragma once

#include "file.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <memory>

namespace Kepler::Compiler {

    namespace Internal {

        llvm::LLVMContext& get_context();
        llvm::IRBuilder<>& get_builder();
        llvm::Module& get_module();
        std::map<std::string, llvm::AllocaInst*>& get_named_values();
        std::unique_ptr<File>& get_file();

    }

    const bool compile_file(const char* filename, const char* outname);

}
