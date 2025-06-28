#pragma once

#include <memory>
#include <string>

#include "../expression.hpp"
#include "../expression_result.hpp"

namespace Kepler::AST {

    class StringLiteralExpression: public Expression {
    private:
        std::string value;

    public:
        StringLiteralExpression(std::string value): value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
