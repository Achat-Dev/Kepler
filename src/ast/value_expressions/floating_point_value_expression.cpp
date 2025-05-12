#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <memory>

#include "../../compiler.hpp"
#include "../../log.hpp"
#include "../../type.hpp"
#include "../expression_result.hpp"
#include "floating_point_value_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> FloatingPointValueExpression::codegen() {
        Type::TypeToken type = Compiler::get_target_type_stack().top();

        if (type == Type::TypeToken::Float32 || type == Type::TypeToken::Float64) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == Type::TypeToken::None) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(Type::TypeToken::Float32), value), Type::TypeToken::Float32, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a float");
        return ExpressionResult::create_invalid();
    }

    double FloatingPointValueExpression::get_value() const {
        return value;
    }

}
