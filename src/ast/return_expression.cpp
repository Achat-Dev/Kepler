#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "return_expression.hpp"

namespace Kepler::AST {

    // TODO: return values need to break from branches
    std::unique_ptr<ExpressionResult> ReturnExpression::codegen() {
        std::unique_ptr<ExpressionResult> value = expression->codegen();
        if (!value->is_valid()) {
            log("Compile error: no valid return value");
            return ExpressionResult::create_invalid();
        }
        if (value->get_status() == ExpressionStatus::NotReturnable) {
            log("Compile error: return value is not returnable");
            return ExpressionResult::create_invalid();
        }

        //Compiler::get_builder().CreateRet(value->get_value());
        //return ExpressionResult::create_not_returnable();
        return ExpressionResult::create_valid(Compiler::get_builder().CreateRet(value->get_value()));
    }

}
