#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"

#include <memory>

namespace Kepler::AST {

    class FloatingPointLiteralExpression: public Expression {
    private:
        double value;

    public:
        FloatingPointLiteralExpression(double value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        double get_value() const;
    };

}
