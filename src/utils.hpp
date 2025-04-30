#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

namespace Kepler {

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* f, llvm::StringRef variable_name);
    void emergency_exit();

}
