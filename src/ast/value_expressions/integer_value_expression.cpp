#include <cstdint>
#include <limits>
#include <llvm/IR/Constants.h>
#include <memory>

#include "integer_value_expression.hpp"
#include "../expression_result.hpp"
#include "../../log.hpp"
#include "../../types/type.hpp"
#include "../../types/target_type_stack.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> IntegerValueExpression::codegen() {
        Type::TypeToken type = Type::TargetTypeStack::top();

        if (Type::is_integer_type(type)) {
            return ExpressionResult::create(llvm::ConstantInt::get(Type::get_by_token(type), value, true), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == Type::TypeToken::None) {
            if (value > std::numeric_limits<int32_t>::max()) {
                return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(Type::TypeToken::Int64), value), Type::TypeToken::Int64, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
            }
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(Type::TypeToken::Int32), value), Type::TypeToken::Int32, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from an integer");
        return ExpressionResult::create_invalid();
    }

    int64_t IntegerValueExpression::get_value() const {
        return value;
    }

}
