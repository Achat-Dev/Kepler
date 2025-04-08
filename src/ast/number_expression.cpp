#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "number_expression.hpp"

namespace Kepler::AST {

    llvm::Value* NumberExpression::codegen() {
        return llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(value));
    }

}
