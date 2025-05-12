#pragma once

#include <cstdint>
#include <memory>

#include "../expression.hpp"
#include "../expression_result.hpp"

namespace Kepler::AST {

    class IntegerValueExpression: public Expression {
    private:
        int64_t value;

    public:
        IntegerValueExpression(int64_t value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        int64_t get_value() const;
    };

}
