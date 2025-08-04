#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"

#include <memory>

namespace Kepler::AST {

    class BooleanLiteralExpression: public Expression {
    private:
        bool value;

    public:
        BooleanLiteralExpression(bool value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
