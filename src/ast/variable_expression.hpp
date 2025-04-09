#pragma once

#include <string>

#include "expression.hpp"
#include "expression_result.hpp"

namespace Kepler::AST {

    class VariableExpression: public Expression {
    private:
        std::string name;

    public:
        VariableExpression(const std::string& name) : name(name) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };


}
