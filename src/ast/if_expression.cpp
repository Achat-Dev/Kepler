#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
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

        // Codegen if body
        Compiler::get_builder().SetInsertPoint(if_block);

        bool if_body_has_return = false;
        for (int i = 0; i < if_body.size(); i++) {
            std::unique_ptr<ExpressionResult> ifv = if_body[i]->codegen();
            if (!ifv->is_valid()) {
                log("Compile error: invalid expression in if body");
                return ExpressionResult::create_invalid();
            }
            if (ifv->is_return_statement() || ifv->forms_qualified_return()) {
                if (if_body.size() - (i + 1) > 0) {
                    log("Compile warning: unreachable code in if body detected");
                }
                if_body_has_return = true;
                break;
            }
        }

        if (!if_body_has_return) {
            Compiler::get_builder().CreateBr(after_branch_block);
        }

        // Codegen else body
        Compiler::get_builder().SetInsertPoint(else_block);

        bool else_body_has_return = false;
        for (int i = 0; i < else_body.size(); i++) {
            std::unique_ptr<ExpressionResult> elsev = else_body[i]->codegen();
            if (!elsev->is_valid()) {
                log("Compile error: invalid expression in else body");
                return ExpressionResult::create_invalid();
            }
            if (elsev->is_return_statement() || elsev->forms_qualified_return()) {
                if (else_body.size() - (i + 1) > 0) {
                    log("Compile warning: unreachable code in else body detected");
                }
                else_body_has_return = true;
                break;
            }
        }

        if (!else_body_has_return) {
            Compiler::get_builder().CreateBr(after_branch_block);
        }

        Compiler::get_builder().SetInsertPoint(after_branch_block);

        unsigned int flags = ExpressionResultFlags::Valid;
        if (if_body_has_return && else_body_has_return) {
            flags |= ExpressionResultFlags::QualifiedReturn;
        }
        return ExpressionResult::create(nullptr, flags);
    }

}
