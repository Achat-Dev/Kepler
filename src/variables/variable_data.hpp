#pragma once

#include <llvm/IR/Instructions.h>

#include "../type.hpp"

namespace Kepler::LocalVariables {

    struct VariableData {
        Type::TypeToken type;
        llvm::AllocaInst* variable;

        VariableData() = default;
        VariableData(Type::TypeToken type, llvm::AllocaInst* alloca): type(type), variable(alloca) {}
    };

}
