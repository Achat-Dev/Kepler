#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"

#include <string>

namespace Kepler::AST {

    class VariableExpression: public Expression {
    private:
        const std::string name;

    public:
        VariableExpression(const std::string& name) : name(name) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };


}
