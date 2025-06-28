#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>

namespace Kepler {

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* f, llvm::Type* type, llvm::StringRef variable_name);
    void emergency_exit(const std::string& message);

}
