#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"

#include <memory>
#include <string>

namespace Kepler::AST {

    class StringLiteralExpression: public Expression {
    private:
        std::string value;

    public:
        StringLiteralExpression(std::string value): value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
