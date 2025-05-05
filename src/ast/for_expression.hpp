#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"
#include "expression_result.hpp"
#include "variable_definition_expression.hpp"

namespace Kepler::AST {

    class ForExpression: public Expression {
    private:
        std::unique_ptr<VariableDefinitionExpression> start;
        std::unique_ptr<Expression> end;
        std::unique_ptr<Expression> step;
        std::vector<std::unique_ptr<Expression>> body;

    public:
        ForExpression(std::unique_ptr<VariableDefinitionExpression> start,
            std::unique_ptr<Expression> end,
            std::unique_ptr<Expression> step,
            std::vector<std::unique_ptr<Expression>> body)
            : start(std::move(start)),
              end(std::move(end)),
              step(std::move(step)),
              body(std::move(body)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
