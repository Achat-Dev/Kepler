#pragma once

#include "ast/expression.hpp"
#include "ast/prototype.hpp"

#include <llvm/IR/Function.h>
#include <memory>
#include <vector>

namespace Kepler::AST {

    class Function {
    private:
        std::shared_ptr<Prototype> prototype;
        std::vector<std::unique_ptr<Expression>> body;

    public:
        Function(std::shared_ptr<Prototype> prototype, std::vector<std::unique_ptr<Expression>> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
        llvm::Function* codegen();
    };


}
