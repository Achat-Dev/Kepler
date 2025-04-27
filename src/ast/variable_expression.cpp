#include <llvm/IR/Instructions.h>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "variable_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableExpression::codegen() {
        llvm::AllocaInst* a = Compiler::get_named_values()[name];
        if (!a) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown variable name", name);
            return ExpressionResult::create_invalid();
        }
        return ExpressionResult::create(Compiler::get_builder().CreateLoad(a->getAllocatedType(), a, name.c_str()), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

    const std::string& VariableExpression::get_name() const {
        return name;
    }

}
