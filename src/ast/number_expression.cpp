#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "expression_result.hpp"
#include "number_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> NumberExpression::codegen() {
        return ExpressionResult::create(llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(value)), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
