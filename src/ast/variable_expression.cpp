#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "variable_expression.hpp"

namespace Kepler::AST {

    llvm::Value* VariableExpression::codegen() {
        llvm::AllocaInst* a = Compiler::Internal::get_named_values()[name];
        if (!a) {
            log("Compile error: unknown variable name", name);
            return nullptr;
        }
        return Compiler::Internal::get_builder().CreateLoad(a->getAllocatedType(), a, name.c_str());
    }

    const std::string& VariableExpression::get_name() const {
        return name;
    }

}
