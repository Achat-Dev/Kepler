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
#include "../types/target_type_stack.hpp"
#include "../variables/local_variables.hpp"
#include "../variables/variable_data.hpp"

namespace Kepler::AST {

    static std::unique_ptr<ExpressionResult> create_less_than(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSLT(lhs->get_value(), rhs->get_value(), "lttmp");
            return ExpressionResult::create(value, Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpULT(lhs->get_value(), rhs->get_value(), "lttmp");
            return ExpressionResult::create(value, Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '<' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

    static std::unique_ptr<ExpressionResult> create_greater_than(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSGT(lhs->get_value(), rhs->get_value(), "gttmp");
            return ExpressionResult::create(value, Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpUGT(lhs->get_value(), rhs->get_value(), "gttmp");
            return ExpressionResult::create(value, Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '>' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

    static std::unique_ptr<ExpressionResult> create_add(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateAdd(lhs->get_value(), rhs->get_value(), "addtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFAdd(lhs->get_value(), rhs->get_value(), "addtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '+' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

    static std::unique_ptr<ExpressionResult> create_sub(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateSub(lhs->get_value(), rhs->get_value(), "subtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFSub(lhs->get_value(), rhs->get_value(), "subtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '-' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

    static std::unique_ptr<ExpressionResult> create_mul(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateMul(lhs->get_value(), rhs->get_value(), "multmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFMul(lhs->get_value(), rhs->get_value(), "multmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '*' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

    static std::unique_ptr<ExpressionResult> create_div(std::unique_ptr<ExpressionResult> lhs, std::unique_ptr<ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateSDiv(lhs->get_value(), rhs->get_value(), "divtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFDiv(lhs->get_value(), rhs->get_value(), "divtmp");
            return ExpressionResult::create(value, type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '/' operation with type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

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

        std::unique_ptr<ExpressionResult> lhs_er = lhs->codegen();
        Type::TargetTypeStack::push(lhs_er->get_type());
        std::unique_ptr<ExpressionResult> rhs_er = rhs->codegen();
        Type::TargetTypeStack::pop();

        if (!lhs_er->is_valid() || !rhs_er->is_valid()) {
            return ExpressionResult::create_invalid();
        }

        if (lhs_er->get_type() != rhs_er->get_type()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to create binary operation with types '", lhs_er->get_type(), "' and '", rhs_er->get_type(), '\'');
            return ExpressionResult::create_invalid();
        }

        llvm::Value* value;
        unsigned int flags = ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable;
        switch (op) {
            case Lexer::Token::LessThan: return create_less_than(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::GreaterThan: return create_greater_than(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Plus: return create_add(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Minus: return create_sub(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Multiplication: return create_mul(std::move(lhs_er), std::move(rhs_er));
            case Lexer::Token::Division: return create_div(std::move(lhs_er), std::move(rhs_er));
            default:
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown binary operator '", op, '\'');
                return ExpressionResult::create_invalid();
        }
    }

    Lexer::Token BinaryExpression::get_operator() const {
        return op;
    }

}
