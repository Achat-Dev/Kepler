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
            VariableExpression* lhs_expression = static_cast<VariableExpression*>(lhs.get());
            if (!lhs_expression) {
                log("Compile error: destination of '=' must be a variable");
                return ExpressionResult::create_invalid();
            }

            std::unique_ptr<ExpressionResult> value = rhs->codegen();
            if (!value->is_valid()) {
                log("Compile error: invalid variable assignment expression");
                return ExpressionResult::create_invalid();
            }

            llvm::Value* variable = Compiler::get_named_values()[lhs_expression->get_name()];

            if (!variable) {
                log("Compile error: unknown variable name:", lhs_expression->get_name());
                return ExpressionResult::create_invalid();
            }

            Compiler::get_builder().CreateStore(value->get_value(), variable);
            return std::move(value);
        }

        std::unique_ptr<ExpressionResult> l = lhs->codegen();
        std::unique_ptr<ExpressionResult> r = rhs->codegen();

        if (!l->is_valid() || !r->is_valid()) {
            return ExpressionResult::create_invalid();
        }

        switch (op) {
            case '<':
                l->set_value(Compiler::get_builder().CreateFCmpULT(l->get_value(), r->get_value(), "cmptmp"));
                return ExpressionResult::create_valid(Compiler::get_builder().CreateUIToFP(l->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp"));
            case '>':
                l->set_value(Compiler::get_builder().CreateFCmpUGT(l->get_value(), r->get_value(), "cmptmp"));
                return ExpressionResult::create_valid(Compiler::get_builder().CreateUIToFP(l->get_value(), llvm::Type::getDoubleTy(Compiler::get_context()), "booltmp"));
            case '+': return ExpressionResult::create_valid(Compiler::get_builder().CreateFAdd(l->get_value(), r->get_value(), "addtmp"));
            case '-': return ExpressionResult::create_valid(Compiler::get_builder().CreateFSub(l->get_value(), r->get_value(), "subtmp"));
            case '*': return ExpressionResult::create_valid(Compiler::get_builder().CreateFMul(l->get_value(), r->get_value(), "multmp"));
            case '/': return ExpressionResult::create_valid(Compiler::get_builder().CreateFDiv(l->get_value(), r->get_value(), "divtmp"));
            default:
                log("Compile error: invalid binary operator");
                return ExpressionResult::create_invalid();
        }
    }

}
