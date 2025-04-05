#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <map>

namespace Kepler::Compiler {

    namespace Internal {

        llvm::LLVMContext& get_context();
        llvm::IRBuilder<>& get_builder();
        llvm::Module& get_module();
        std::map<std::string, llvm::Value*>& get_named_values();

    }

    const bool compile_file(const char* filename, const char* outname);

}
