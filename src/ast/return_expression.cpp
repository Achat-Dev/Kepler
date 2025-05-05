#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "return_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> ReturnExpression::codegen() {
        std::unique_ptr<ExpressionResult> expression_er = expression->codegen();
        if (!expression_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": no valid return value");
            return ExpressionResult::create_invalid();
        }
        if (!expression_er->is_returnable()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": return value is not returnable");
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(Compiler::get_builder().CreateRet(expression_er->get_value()), ExpressionResultFlags::Valid | ExpressionResultFlags::Return);
    }

}
