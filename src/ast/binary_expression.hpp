#pragma once

#include "expression.hpp"
#include "expression_result.hpp"
#include "lexer.hpp"

#include <memory>

namespace Kepler::AST {

    class BinaryExpression : public Expression {
    private:
        Lexer::Token op;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

    public:
        BinaryExpression(Lexer::Token op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        Lexer::Token get_operator() const;
    };

}
