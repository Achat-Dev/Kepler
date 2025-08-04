#include "ast/return_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "function_registry/function_registry.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> ReturnExpression::codegen() {
        Type::TypeToken target_return_type = FunctionRegistry::get_current_prototype()->get_type();

        if (target_return_type == Type::TypeToken::Void) {
            return ExpressionResult::create(Compiler::get_builder().CreateRetVoid(), Type::TypeToken::Void, ExpressionResultFlags::Valid | ExpressionResultFlags::Return);
        }

        Type::TargetTypeStack::push(target_return_type);

        std::unique_ptr<ExpressionResult> expression_er = expression->codegen();
        if (!expression_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid return value");
            return ExpressionResult::create_invalid();
        }
        if (!expression_er->is_returnable()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": return value is not returnable");
            return ExpressionResult::create_invalid();
        }

        if (expression_er->get_type() != target_return_type) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to return a value of type '", expression_er->get_type(), "' from a function of type '", target_return_type, '\'');
            return ExpressionResult::create_invalid();
        }

        Type::TargetTypeStack::pop();

        return ExpressionResult::create(Compiler::get_builder().CreateRet(expression_er->get_value()), expression_er->get_type(), ExpressionResultFlags::Valid | ExpressionResultFlags::Return);
    }

}
