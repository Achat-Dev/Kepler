#include "ast/cast_expression.hpp"

#include "ast/expression_result.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> CastExpression::codegen() {
        // Push 'None' as the target type in order to let value expressions choose their default type
        Type::TargetTypeStack::push(Type::TypeToken::None);

        std::unique_ptr<ExpressionResult> value_er = value->codegen();

        if (!value_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in cast");
            return ExpressionResult::create_invalid();
        }
        if (!value_er->is_returnable()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": cannot cast expression, because it has no value");
            return ExpressionResult::create_invalid();
        }

        Type::TargetTypeStack::pop();

        llvm::Value* result = Type::cast(value_er->get_value(), value_er->get_type(), type);
        if (!result) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to cast value from type '", value_er->get_type(), "' to type '", type, '\'');
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(result, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
