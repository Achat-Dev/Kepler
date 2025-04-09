#pragma once

#include <memory>
#include <utility>

#include "expression.hpp"

namespace Kepler::AST {

    class ReturnExpression : public Expression {
    private:
        std::unique_ptr<Expression> expression;

    public:
        ReturnExpression(std::unique_ptr<Expression> expression) : expression(std::move(expression)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
