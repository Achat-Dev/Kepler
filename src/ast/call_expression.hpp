#pragma once

#include "ast/expression.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Kepler::AST {

    class CallExpression: public Expression {
    private:
        std::string callee;
        std::vector<std::unique_ptr<Expression>> args;

    public:
        CallExpression(const std::string& callee, std::vector<std::unique_ptr<Expression>> args)
            : callee(callee), args(std::move(args)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };


}
