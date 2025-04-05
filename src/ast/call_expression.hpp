#pragma once

#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

#include "expression.hpp"

namespace Kepler::AST {

    class CallExpression : public Expression {
    private:
        std::string callee;
        std::vector<std::unique_ptr<Expression>> args;

    public:
        CallExpression(const std::string& callee, std::vector<std::unique_ptr<Expression>> args)
            : callee(callee), args(std::move(args)) {}
        llvm::Value* codegen() override;
    };


}
