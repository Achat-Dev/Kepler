#pragma once

#include <memory>

#include "../expression.hpp"
#include "../expression_result.hpp"

namespace Kepler::AST {

    class BooleanLiteralExpression: public Expression {
    private:
        bool value;

    public:
        BooleanLiteralExpression(bool value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
