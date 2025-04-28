#pragma once

#include <memory>
#include <string>

#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class VariableDefinitionExpression: public Expression {
    private:
        std::unique_ptr<Expression> value;
        const std::string name;

    public:
        VariableDefinitionExpression(const std::string& name, std::unique_ptr<Expression> value)
            : name(name), value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };

}
