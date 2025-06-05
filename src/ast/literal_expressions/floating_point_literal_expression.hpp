#pragma once

#include <memory>

#include "../expression.hpp"
#include "../expression_result.hpp"

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
