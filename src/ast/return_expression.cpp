#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "return_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> ReturnExpression::codegen() {
        Compiler::get_target_type_stack().push(Compiler::get_function_return_type());

        std::unique_ptr<ExpressionResult> expression_er = expression->codegen();
        if (!expression_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": no valid return value");
            return ExpressionResult::create_invalid();
        }
        if (!expression_er->is_returnable()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": return value is not returnable");
            return ExpressionResult::create_invalid();
        }

        if (expression_er->get_type() != Compiler::get_function_return_type()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to return a value of type '", expression_er->get_type(), "' from a function of type '", Compiler::get_function_return_type(), '\'');
            return ExpressionResult::create_invalid();
        }

        Compiler::get_target_type_stack().pop();

        return ExpressionResult::create(Compiler::get_builder().CreateRet(expression_er->get_value()), expression_er->get_type(), ExpressionResultFlags::Valid | ExpressionResultFlags::Return);
    }

}
