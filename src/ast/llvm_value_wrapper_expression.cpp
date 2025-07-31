#include <memory>

#include "expression_result.hpp"
#include "llvm_value_wrapper_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> LLVMValueWrapperExpression::codegen() {
        return ExpressionResult::create(value, type, flags);
    }

}
