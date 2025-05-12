#include <cstdint>
#include <llvm/IR/Constants.h>
#include <memory>

#include "../../compiler.hpp"
#include "../../log.hpp"
#include "../../type.hpp"
#include "../expression_result.hpp"
#include "integer_value_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> IntegerValueExpression::codegen() {
        TypeToken type = Compiler::get_target_type_stack().top();

        if (type == TypeToken::Int8 || type == TypeToken::Int16 || type == TypeToken::Int32 || type == TypeToken::Int64) {
            return ExpressionResult::create(llvm::ConstantInt::get(Type::get_by_token(type), value, true), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == TypeToken::Float32 || type == TypeToken::Float64) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == TypeToken::None) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(TypeToken::Int32), value), TypeToken::Int32, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from an integer");
        return ExpressionResult::create_invalid();
    }

    int64_t IntegerValueExpression::get_value() const {
        return value;
    }

}
