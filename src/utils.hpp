#pragma once

#include "types/type_token.hpp"

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace Kepler {

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* f, Type::TypeToken type, llvm::StringRef variable_name);
    void throw_runtime_error(const std::string& message);
    void emergency_exit(const std::string& message);

}
