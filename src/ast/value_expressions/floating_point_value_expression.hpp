#pragma once

#include <memory>

#include "../expression.hpp"
#include "../expression_result.hpp"

namespace Kepler::AST {

    class FloatingPointValueExpression: public Expression {
    private:
        double value;

    public:
        FloatingPointValueExpression(double value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        double get_value() const;
    };

}
