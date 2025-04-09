#pragma once

#include <memory>

#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class BinaryExpression : public Expression {
    private:
        char op;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

    public:
        BinaryExpression(char op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
