#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "types/type_token.hpp"

#include <memory>
#include <utility>

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
