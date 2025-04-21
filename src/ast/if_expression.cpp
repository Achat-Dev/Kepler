#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "expression_result.hpp"
#include "if_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> IfExpression::codegen() {
        std::unique_ptr<ExpressionResult> conditionv = condition->codegen();
        if (!conditionv->is_valid()) {
            return ExpressionResult::create_invalid();
        }

        // Make conditional a bool by comparing it to 0.0
        conditionv->set_value(Compiler::get_builder().CreateFCmpONE(conditionv->get_value(), llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(0.0)), "ifcond"));

        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();

        llvm::BasicBlock* if_block = llvm::BasicBlock::Create(Compiler::get_context(), "ifbranch", f);
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(Compiler::get_context(), "elsebranch", f);
        llvm::BasicBlock* after_branch_block = llvm::BasicBlock::Create(Compiler::get_context(), "afterbranch", f);

        Compiler::get_builder().CreateCondBr(conditionv->get_value(), if_block, else_block);
        Compiler::get_builder().SetInsertPoint(if_block);

        std::unique_ptr<ExpressionResult> ifv = if_branch->codegen();
        if (!ifv->is_valid()) {
            return ExpressionResult::create_invalid();
        }
        if (!ifv->is_return_statement()) {
            Compiler::get_builder().CreateBr(after_branch_block);
        }

        Compiler::get_builder().SetInsertPoint(else_block);

        std::unique_ptr<ExpressionResult> elsev = else_branch->codegen();
        if (!elsev->is_valid()) {
            return ExpressionResult::create_invalid();
        }
        if (!elsev->is_return_statement()) {
            Compiler::get_builder().CreateBr(after_branch_block);
        }

        Compiler::get_builder().SetInsertPoint(after_branch_block);

        unsigned int flags = ExpressionResultFlags::Valid;
        if (ifv->is_return_statement() && elsev->is_return_statement()) {
            flags |= ExpressionResultFlags::QualifiedReturn;
        }
        return ExpressionResult::create(nullptr, flags);
    }

}
