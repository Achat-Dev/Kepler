#pragma once

#include "ast/binary_expression.hpp"
#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "types/type_token.hpp"

#include <memory>
#include <string>

namespace Kepler::AST {

    class VariableDefinitionExpression: public Expression {
    private:
        const Type::TypeToken type;
        const std::string name;
        std::unique_ptr<BinaryExpression> value;

    public:
        VariableDefinitionExpression(Type::TypeToken type, const std::string& name, std::unique_ptr<BinaryExpression> value)
            : type(type), name(name), value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };

}
