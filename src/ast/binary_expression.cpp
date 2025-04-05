#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "../log.hpp"
#include "binary_expression.hpp"

namespace Kepler::AST {

    llvm::Value* BinaryExpression::codegen() {
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
