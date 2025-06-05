#include <limits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <memory>

#include "floating_point_literal_expression.hpp"
#include "../expression_result.hpp"
#include "../../log.hpp"
#include "../../types/type.hpp"
#include "../../types/target_type_stack.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> FloatingPointLiteralExpression::codegen() {
        Type::TypeToken type = Type::TargetTypeStack::top();

        if (Type::is_floating_point_type(type)) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == Type::TypeToken::None) {
            if (value > std::numeric_limits<float>::max()) {
                return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(Type::TypeToken::Float64), value), Type::TypeToken::Float64, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
            }
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(Type::TypeToken::Float32), value), Type::TypeToken::Float32, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a floating point value");
        return ExpressionResult::create_invalid();
    }

    double FloatingPointLiteralExpression::get_value() const {
        return value;
    }

}
