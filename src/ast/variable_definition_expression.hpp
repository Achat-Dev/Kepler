#pragma once

#include <memory>
#include <string>

#include "binary_expression.hpp"
#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class VariableDefinitionExpression: public Expression {
    private:
        std::unique_ptr<BinaryExpression> value;
        const std::string name;

    public:
        VariableDefinitionExpression(const std::string& name, std::unique_ptr<BinaryExpression> value)
            : name(name), value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
