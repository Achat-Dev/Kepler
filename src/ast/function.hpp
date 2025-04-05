#pragma once

#include <llvm/IR/Function.h>
#include <memory>

#include "expression.hpp"
#include "prototype.hpp"

namespace Kepler::AST {

    class Function {
    private:
        std::unique_ptr<Prototype> prototype;
        std::unique_ptr<Expression> body;

    public:
        Function(std::unique_ptr<Prototype> prototype, std::unique_ptr<Expression> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
        llvm::Function* codegen();
    };


}
