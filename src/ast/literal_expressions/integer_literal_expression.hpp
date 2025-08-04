#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"

#include <cstdint>
#include <memory>

namespace Kepler::AST {

    class IntegerLiteralExpression: public Expression {
    private:
        int64_t value;

    public:
        IntegerLiteralExpression(int64_t value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        int64_t get_value() const;
    };

}
