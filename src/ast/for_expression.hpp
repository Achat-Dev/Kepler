#pragma once

#include <memory>
#include <string>

#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class ForExpression: public Expression {
    private:
        std::string variable_name;
        std::unique_ptr<Expression> start, end, step, body;

    public:
        ForExpression(std::string variable_name,
            std::unique_ptr<Expression> start,
            std::unique_ptr<Expression> end,
            std::unique_ptr<Expression> step,
            std::unique_ptr<Expression> body)
            : variable_name(variable_name),
              start(std::move(start)),
              end(std::move(end)),
              step(std::move(step)),
              body(std::move(body)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
