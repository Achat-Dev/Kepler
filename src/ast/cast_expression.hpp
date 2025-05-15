#pragma once

#include <memory>
#include <utility>

#include "expression.hpp"
#include "expression_result.hpp"
#include "../types/type.hpp"

namespace Kepler::AST {

    class CastExpression: public Expression {
    private:
        std::unique_ptr<Expression> value;
        Type::TypeToken type;

    public:
        CastExpression(std::unique_ptr<Expression> value, Type::TypeToken type)
            : value(std::move(value)), type(type) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
