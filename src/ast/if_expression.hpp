#pragma once

#include <llvm/IR/Value.h>
#include <memory>

#include "expression.hpp"

namespace Kepler::AST {

    class IfExpression: public Expression {
    private:
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Expression> if_branch;
        std::unique_ptr<Expression> else_branch;

    public:
        IfExpression(std::unique_ptr<Expression> condition,
            std::unique_ptr<Expression> if_branch,
            std::unique_ptr<Expression> else_branch)
            : condition(std::move(condition)),
              if_branch(std::move(if_branch)),
              else_branch(std::move(else_branch)) {}
        llvm::Value* codegen() override;
    };

}
