#pragma once

#include <memory>

#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class NegationExpression: public Expression {
    private:
        std::unique_ptr<Expression> value;

    public:
        NegationExpression(std::unique_ptr<Expression> value): value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
