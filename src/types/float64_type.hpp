#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"

namespace Kepler::Type::Float64Type {

    llvm::Type* get_llvm_type();
    llvm::Value* cast(llvm::Value* value, TypeToken to);
    std::string get_name();

}
