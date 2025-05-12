#pragma once

#include <llvm/IR/Instructions.h>

#include "../type.hpp"

namespace Kepler::AST {

    struct VariableData {
        const TypeToken type;
        llvm::AllocaInst* variable;

        VariableData(TypeToken type, llvm::AllocaInst* alloca): type(type), variable(alloca) {}
    };

}
