#include "ast/llvm_value_wrapper_expression.hpp"

#include "ast/expression_result.hpp"

#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> LLVMValueWrapperExpression::codegen() {
        return ExpressionResult::create(value, type, flags);
    }

}
