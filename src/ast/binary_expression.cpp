#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "../log.hpp"
#include "variable_expression.hpp"
#include "binary_expression.hpp"

namespace Kepler::AST {

    llvm::Value* BinaryExpression::codegen() {
        if (op == '=') {
            VariableExpression* lhs_expression = static_cast<VariableExpression*>(lhs.get());
            if (!lhs_expression) {
                return log_errorv("Destination of '=' must be a variable");
            }

            llvm::Value* value = rhs->codegen();
            if (!value) {
                return nullptr;
            }

            llvm::Value* variable = Compiler::Internal::get_named_values()[lhs_expression->get_name()];

            if (!variable) {
                return log_errorv("Unknown variable name " + lhs_expression->get_name());
            }

            Compiler::Internal::get_builder().CreateStore(value, variable);
            return value;
        }

        llvm::Value* l = lhs->codegen();
        llvm::Value* r = rhs->codegen();

        if (!l || !r) {
            return nullptr;
        }

        switch (op) {
            case '<':
                l = Compiler::Internal::get_builder().CreateFCmpULT(l, r, "cmptmp");
                return Compiler::Internal::get_builder().CreateUIToFP(l, llvm::Type::getDoubleTy(Compiler::Internal::get_context()), "booltmp");
            case '>':
                l = Compiler::Internal::get_builder().CreateFCmpUGT(l, r, "cmptmp");
                return Compiler::Internal::get_builder().CreateUIToFP(l, llvm::Type::getDoubleTy(Compiler::Internal::get_context()), "booltmp");
            case '+': return Compiler::Internal::get_builder().CreateFAdd(l, r, "addtmp");
            case '-': return Compiler::Internal::get_builder().CreateFSub(l, r, "subtmp");
            case '*': return Compiler::Internal::get_builder().CreateFMul(l, r, "multmp");
            case '/': return Compiler::Internal::get_builder().CreateFDiv(l, r, "divtmp");
            default: return log_errorv("invalid binary operator");
        }
    }

}
