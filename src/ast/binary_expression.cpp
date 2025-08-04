#include "ast/binary_expression.hpp"

#include "ast/expression_result.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"
#include "variable_expression.hpp"
#include "variables/local_variables.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <optional>
#include <utility>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> BinaryExpression::codegen() {
        if (op == Lexer::Token::Assignment) {
            VariableExpression* lhs_as_variable = dynamic_cast<VariableExpression*>(lhs.get());
            if (!lhs_as_variable) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": destination of '=' must be a variable");
                return ExpressionResult::create_invalid();
            }

            // Don't codegen the lhs expression because we know it's a VariableExpression and we don't want to unnecessarily load it
            // Just do a lookup to check if the variable name exists
            std::optional<LocalVariables::VariableData> variable_data = LocalVariables::get(lhs_as_variable->get_name());
            if (!variable_data) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown variable name '", lhs_as_variable->get_name(), '\'');
                return ExpressionResult::create_invalid();
            }

            Type::TargetTypeStack::push(variable_data->type);

            std::unique_ptr<ExpressionResult> rhs_er = rhs->codegen();
            if (!rhs_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid assignment expression to variable '", lhs_as_variable->get_name(), '\'');
                return ExpressionResult::create_invalid();
            }
            if (!rhs_er->is_assignable()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": assignment value to variable '", lhs_as_variable->get_name(), "' is not assignable");
                return ExpressionResult::create_invalid();
            }

            if (rhs_er->get_type() != variable_data->type && variable_data->type != Type::TypeToken::TMap) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to assign a value of type '", rhs_er->get_type(), "' to a variable of type '", variable_data->type, '\'');
                return ExpressionResult::create_invalid();
            }

            Type::TargetTypeStack::pop();

            bool was_assignment_successful;
            if (variable_data->type == Type::TypeToken::TMap) {
                was_assignment_successful = Type::create_assign(rhs_er->get_value(), rhs_er->get_type(), variable_data.value());
            }
            else {
                was_assignment_successful = Type::create_assign(rhs_er->get_value(), variable_data.value());
            }

            if (!was_assignment_successful) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to create assignment to variable of type '", variable_data->type, '\'');
                return nullptr;
            }

            return std::move(rhs_er);
        }

        // Check if the current target type is TypeToken::Bool
        // If so, push TypeToken::None onto the TargetTypeStack to let the lhs expression choose its default type
        // This is needed because some binary operations can create a boolean, but the lhs expression has to evaluate to something other than boolean
        bool should_target_default_type = Type::TargetTypeStack::top() == Type::TypeToken::Bool;
        if (should_target_default_type) {
            Type::TargetTypeStack::push(Type::TypeToken::None);
        }

        // Codegen lhs
        std::unique_ptr<ExpressionResult> lhs_er = lhs->codegen();
        if (should_target_default_type) {
            Type::TargetTypeStack::pop();
        }

        if (!lhs_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid left-hand side in binary expression");
            return ExpressionResult::create_invalid();
        }

        // Codegen rhs
        Type::TypeToken lhs_type = lhs_er->get_type();
        Type::TargetTypeStack::push(lhs_type);
        std::unique_ptr<ExpressionResult> rhs_er = rhs->codegen();
        Type::TargetTypeStack::pop();

        if (!rhs_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid right-hand side in binary expression");
            return ExpressionResult::create_invalid();
        }

        if (lhs_type != rhs_er->get_type()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to create binary operation with types '", lhs_er->get_type(), "' and '", rhs_er->get_type(), '\'');
            return ExpressionResult::create_invalid();
        }

        // Create binary operation
        llvm::Value* value;
        Type::TypeToken return_type = lhs_type;

        switch (op) {
            case Lexer::Token::Plus:
                value = Type::create_add(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                break;
            case Lexer::Token::Minus:
                value = Type::create_sub(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                break;
            case Lexer::Token::Multiplication:
                value = Type::create_mul(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                break;
            case Lexer::Token::Division:
                value = Type::create_div(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                break;
            case Lexer::Token::LessThan:
                value = Type::create_less_than(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            case Lexer::Token::GreaterThan:
                value = Type::create_greater_than(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            case Lexer::Token::Equals:
                value = Type::create_equals(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            case Lexer::Token::NotEquals:
                value = Type::create_not_equals(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            case Lexer::Token::LessEquals:
                value = Type::create_less_equals(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            case Lexer::Token::GreaterEquals:
                value = Type::create_greater_equals(lhs_er->get_value(), rhs_er->get_value(), lhs_type);
                return_type = Type::TypeToken::Bool;
                break;
            default:
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown binary operator '", op, '\'');
                return ExpressionResult::create_invalid();
        }

        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to create binary operation between type '", lhs_type, '\'');
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(value, return_type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

    Lexer::Token BinaryExpression::get_operator() const {
        return op;
    }

}
