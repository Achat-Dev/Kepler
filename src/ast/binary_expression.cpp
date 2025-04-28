#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "binary_expression.hpp"
#include "expression_result.hpp"
#include "variable_expression.hpp"

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
            llvm::Value* variable = Compiler::get_named_values()[lhs_as_variable->get_name()];
            if (!variable) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown variable name '", lhs_as_variable->get_name(), '\'');
                return ExpressionResult::create_invalid();
            }

            std::unique_ptr<ExpressionResult> valuev = rhs->codegen();
            if (!valuev->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid assignment expression to variable '", lhs_as_variable->get_name(), '\'');
                return ExpressionResult::create_invalid();
            }
            if (!valuev->is_assignable()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": assignment value to variable '", lhs_as_variable->get_name(), "' is not assignable");
                return ExpressionResult::create_invalid();
            }

            Compiler::get_builder().CreateStore(valuev->get_value(), variable);
            return std::move(valuev);
        }

        std::unique_ptr<ExpressionResult> lhsv = lhs->codegen();
        std::unique_ptr<ExpressionResult> rhsv = rhs->codegen();

        if (!lhsv->is_valid() || !rhsv->is_valid()) {
            return ExpressionResult::create_invalid();
        }

        llvm::Value* value;
        unsigned int flags = ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable;
        switch (op) {
            case '<':
                lhsv->set_value(Compiler::get_builder().CreateFCmpULT(lhsv->get_value(), rhsv->get_value(), "cmptmp"));
                value = Compiler::get_builder().CreateUIToFP(lhsv->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp");
                return ExpressionResult::create(value, flags);
            case '>':
                lhsv->set_value(Compiler::get_builder().CreateFCmpUGT(lhsv->get_value(), rhsv->get_value(), "cmptmp"));
                value = Compiler::get_builder().CreateUIToFP(lhsv->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp");
                return ExpressionResult::create(value, flags);
            case '+':
                value = Compiler::get_builder().CreateFAdd(lhsv->get_value(), rhsv->get_value(), "addtmp");
                return ExpressionResult::create(value, flags);
            case '-':
                value = Compiler::get_builder().CreateFSub(lhsv->get_value(), rhsv->get_value(), "subtmp");
                return ExpressionResult::create(value, flags);
            case '*':
                value = Compiler::get_builder().CreateFMul(lhsv->get_value(), rhsv->get_value(), "multmp");
                return ExpressionResult::create(value, flags);
            case '/':
                value = Compiler::get_builder().CreateFDiv(lhsv->get_value(), rhsv->get_value(), "divtmp");
                return ExpressionResult::create(value, flags);
            default:
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid binary operator");
                return ExpressionResult::create_invalid();
        }
    }

    char BinaryExpression::get_operator() const {
        return op;
    }

}
