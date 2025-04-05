#pragma once

#include <llvm/IR/Value.h>
#include <string>

#include "expression.hpp"

namespace Kepler::AST {

    class VariableExpression: public Expression {
    private:
        std::string name;

    public:
        VariableExpression(const std::string& name) : name(name) {}
        llvm::Value* codegen() override;
    };


}
