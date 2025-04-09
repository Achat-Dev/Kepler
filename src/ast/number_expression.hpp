#pragma once

#include <memory>

#include "expression.hpp"

namespace Kepler::AST {

    class NumberExpression: public Expression {
    private:
        double value;

    public:
        NumberExpression(double value) : value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
