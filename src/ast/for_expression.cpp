#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../utils.hpp"
#include "expression.hpp"
#include "expression_result.hpp"
#include "for_expression.hpp"

namespace Kepler::AST {

    static std::unique_ptr<ExpressionResult> codegen_end_condition(std::unique_ptr<Expression>& end) {
        std::unique_ptr<ExpressionResult> end_condition = end->codegen();
        if (!end_condition->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' end condition");
            return ExpressionResult::create_invalid();
        }

        // Convert condition to a bool by comparing to 0.0
        end_condition->set_value(Compiler::get_builder().CreateFCmpONE(end_condition->get_value(), llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(0.0)), "loopcondition"));
        return std::move(end_condition);
    }

    std::unique_ptr<ExpressionResult> ForExpression::codegen() {
        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();
        llvm::AllocaInst* alloca = create_entry_block_alloca(f, variable_name);

        // Save old variable if the loop variable overrides it
        llvm::AllocaInst* old_value = Compiler::get_named_values()[variable_name];
        Compiler::get_named_values()[variable_name] = alloca;

        // Codegen start value
        std::unique_ptr<ExpressionResult> startv = start->codegen();
        if (!startv->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' start value");
            return ExpressionResult::create_invalid();
        }
        Compiler::get_builder().CreateStore(startv->get_value(), alloca);

        // Codegen the end condition
        // This needs to be codegened in the entry block of the function in order to determine if the loop should be entered at all
        std::unique_ptr<ExpressionResult> start_condition = codegen_end_condition(end);

        // Don't create a terminator for the entry block yet because that depends on if the body is a qualified return
        llvm::BasicBlock* entry_block = Compiler::get_builder().GetInsertBlock();
        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "loop", f);
        Compiler::get_builder().SetInsertPoint(loop_block);

        // Codegen body
        for (int i = 0; i < body.size(); i++) {
            std::unique_ptr<ExpressionResult> bodyv = body[i]->codegen();
            if (!bodyv->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' body");
                return ExpressionResult::create_invalid();
            }
            if (bodyv->is_return_statement() || bodyv->forms_qualified_return()) {
                log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": return in 'for' body detected. This will always be executed even if the loop condition is false at start. Consider removing the for loop");

                // Create the terminator for the entry block -> branch to the loop_block
                Compiler::get_builder().SetInsertPoint(entry_block);
                Compiler::get_builder().CreateBr(loop_block);
                Compiler::get_builder().SetInsertPoint(loop_block);

                return ExpressionResult::create(nullptr, ExpressionResultFlags::Valid | ExpressionResultFlags::QualifiedReturn);
            }
        }

        // Codegen step
        std::unique_ptr<ExpressionResult> step_value = nullptr;
        if (step) {
            step_value = step->codegen();
            if (!step_value->is_valid()) {
                return ExpressionResult::create_invalid();
            }
        }
        else {
            step_value->set_value(llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(1.0)));
        }

        // Calculate loop variable for next iteration
        llvm::Value* current_variable = Compiler::get_builder().CreateLoad(alloca->getAllocatedType(), alloca, variable_name.c_str());
        llvm::Value* next_variable = Compiler::get_builder().CreateFAdd(current_variable, step_value->get_value(), "nextvariable");
        Compiler::get_builder().CreateStore(next_variable, alloca);

        // Codegen loop end condition in the body to check if the loop should be exited
        std::unique_ptr<ExpressionResult> end_condition = codegen_end_condition(end);

        //Evaluate if loop should exit
        llvm::BasicBlock* after_loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "afterloop", f);
        Compiler::get_builder().CreateCondBr(end_condition->get_value(), loop_block, after_loop_block);

        // Create the terminator for the entry block -> conditional branch to either the loop_block or after_loop_block
        Compiler::get_builder().SetInsertPoint(entry_block);
        Compiler::get_builder().CreateCondBr(start_condition->get_value(), loop_block, after_loop_block);

        Compiler::get_builder().SetInsertPoint(after_loop_block);

        // Restore old variable if necessary
        if (old_value) {
            Compiler::get_named_values()[variable_name] = old_value;
        }
        else {
            Compiler::get_named_values().erase(variable_name);
        }

        return ExpressionResult::create(nullptr, ExpressionResultFlags::Valid);
    }

}
