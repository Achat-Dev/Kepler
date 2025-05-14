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

    std::unique_ptr<ExpressionResult> BinaryExpression::codegen() {
        if (op == '=') {
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
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: trying to assign a value of type '", rhs_er->get_type(), "' to variable of type '", variable_data->type, '\'');
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
            case '<':
                lhs_er->set_value(Compiler::get_builder().CreateFCmpULT(lhs_er->get_value(), rhs_er->get_value(), "cmptmp"));
                value = Compiler::get_builder().CreateUIToFP(lhs_er->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            case '>':
                lhs_er->set_value(Compiler::get_builder().CreateFCmpUGT(lhs_er->get_value(), rhs_er->get_value(), "cmptmp"));
                value = Compiler::get_builder().CreateUIToFP(lhs_er->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            case '+':
                value = Compiler::get_builder().CreateFAdd(lhs_er->get_value(), rhs_er->get_value(), "addtmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            case '-':
                value = Compiler::get_builder().CreateFSub(lhs_er->get_value(), rhs_er->get_value(), "subtmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            case '*':
                value = Compiler::get_builder().CreateFMul(lhs_er->get_value(), rhs_er->get_value(), "multmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            case '/':
                value = Compiler::get_builder().CreateFDiv(lhs_er->get_value(), rhs_er->get_value(), "divtmp");
                return ExpressionResult::create(value, lhs_er->get_type(), flags);
            default:
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid binary operator");
                return ExpressionResult::create_invalid();
        }
    }

    char BinaryExpression::get_operator() const {
        return op;
    }

}
