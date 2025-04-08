#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "if_expression.hpp"

namespace Kepler::AST {

    llvm::Value* IfExpression::codegen() {
        llvm::Value* conditionv = condition->codegen();
        if (!conditionv) {
            return nullptr;
        }

        // Make conditional a bool by comparing it to 0.0
        conditionv = Compiler::get_builder().CreateFCmpONE(conditionv, llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(0.0)), "ifcond");

        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();

        llvm::BasicBlock* if_block = llvm::BasicBlock::Create(Compiler::get_context(), "ifbranch", f);
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(Compiler::get_context(), "elsebranch");
        llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(Compiler::get_context(), "mergebranch");

        Compiler::get_builder().CreateCondBr(conditionv, if_block, else_block);

        Compiler::get_builder().SetInsertPoint(if_block);
        llvm::Value* ifv = if_branch->codegen();
        if (!ifv) {
            return nullptr;
        }

        Compiler::get_builder().CreateBr(merge_block);
        if_block = Compiler::get_builder().GetInsertBlock();

        f->insert(f->end(), else_block);
        Compiler::get_builder().SetInsertPoint(else_block);

        llvm::Value* elsev = else_branch->codegen();
        if (!elsev) {
            return nullptr;
        }

        Compiler::get_builder().CreateBr(merge_block);
        else_block = Compiler::get_builder().GetInsertBlock();

        f->insert(f->end(), merge_block);
        Compiler::get_builder().SetInsertPoint(merge_block);
        llvm::PHINode* phi = Compiler::get_builder().CreatePHI(llvm::Type::getDoubleTy(Compiler::get_context()), 2, "iftmp");
        phi->addIncoming(ifv, if_block);
        phi->addIncoming(elsev, else_block);
        return phi;
    }

}
