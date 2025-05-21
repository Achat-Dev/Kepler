#pragma once

#include <memory>

#include "../expression.hpp"
#include "../expression_result.hpp"

namespace Kepler::AST {

    class BooleanValueExpression: public Expression {
    private:
        bool value;

    public:
        BooleanValueExpression(bool value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
