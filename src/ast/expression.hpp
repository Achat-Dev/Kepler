#pragma once

#include <llvm/IR/Value.h>

namespace Kepler::AST {

    class Expression {
    public:
        virtual ~Expression() = default;
        virtual llvm::Value* codegen() = 0;
    };

}
