#pragma once

#include <llvm/IR/Value.h>

#include "expression.hpp"

namespace Kepler::AST {

    class NumberExpression: public Expression {
    private:
        double value;

    public:
        NumberExpression(double value) : value(value) {}
        llvm::Value* codegen() override;
    };

}
