#pragma once

#include "types/type_token.hpp"

#include <llvm/IR/Instructions.h>

namespace Kepler::LocalVariables {

    struct VariableData {
        Type::TypeToken type;
        llvm::AllocaInst* variable;

        VariableData() = default;
        VariableData(Type::TypeToken type, llvm::AllocaInst* alloca): type(type), variable(alloca) {}
    };

}
