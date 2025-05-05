#pragma once

#include <memory>
#include <string>

#include "../type.hpp"
#include "binary_expression.hpp"
#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class VariableDefinitionExpression: public Expression {
    private:
        const TypeToken type;
        const std::string name;
        std::unique_ptr<BinaryExpression> value;

    public:
        VariableDefinitionExpression(TypeToken type, const std::string& name, std::unique_ptr<BinaryExpression> value)
            : type(type), name(name), value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };

}
