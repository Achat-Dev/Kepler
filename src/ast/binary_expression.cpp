#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <optional>
#include <utility>

#include "binary_expression.hpp"
#include "expression_result.hpp"
#include "variable_expression.hpp"
#include "../compiler.hpp"
#include "../log.hpp"
#include "../operators/operators.hpp"
#include "../types/target_type_stack.hpp"
#include "../variables/local_variables.hpp"
#include "../variables/variable_data.hpp"

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

            if (rhs_er->get_type() != variable_data->type) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to assign a value of type '", rhs_er->get_type(), "' to a variable of type '", variable_data->type, '\'');
                return ExpressionResult::create_invalid();
            }

            Type::TargetTypeStack::pop();

            Compiler::get_builder().CreateStore(rhs_er->get_value(), variable_data->variable);
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
        Type::TargetTypeStack::push(lhs_er->get_type());
        std::unique_ptr<ExpressionResult> rhs_er = rhs->codegen();
        Type::TargetTypeStack::pop();

        if (!rhs_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid right-hand side in binary expression");
            return ExpressionResult::create_invalid();
        }

        if (lhs_er->get_type() != rhs_er->get_type()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to create binary operation with types '", lhs_er->get_type(), "' and '", rhs_er->get_type(), '\'');
            return ExpressionResult::create_invalid();
        }

        // Create binary operation
        llvm::Value* value;
        unsigned int flags = ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable;
        switch (op) {
            case Lexer::Token::Plus: return Operators::create_add(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Minus: return Operators::create_sub(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Multiplication: return Operators::create_mul(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Division: return Operators::create_div(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::LessThan: return Operators::create_less_than(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::GreaterThan: return Operators::create_greater_than(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Equals: return Operators::create_equals(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::NotEquals: return Operators::create_not_equals(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::LessEquals: return Operators::create_less_equals(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::GreaterEquals: return Operators::create_greater_equals(std::move(lhs_er), std::move(rhs_er));
            default:
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown binary operator '", op, '\'');
                return ExpressionResult::create_invalid();
        }
    }

    Lexer::Token BinaryExpression::get_operator() const {
        return op;
    }

}
