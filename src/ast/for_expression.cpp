#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "for_expression.hpp"

namespace Kepler::AST {

    llvm::Value* ForExpression::codegen() {
        llvm::Value* startv = start->codegen();
        if (!startv) {
            return nullptr;
        }

        llvm::Function* f = Compiler::Internal::get_builder().GetInsertBlock()->getParent();
        llvm::BasicBlock* preheader_block = Compiler::Internal::get_builder().GetInsertBlock();
        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(Compiler::Internal::get_context(), "loop", f);

        Compiler::Internal::get_builder().CreateBr(loop_block);
        Compiler::Internal::get_builder().SetInsertPoint(loop_block);

        llvm::PHINode* variable = Compiler::Internal::get_builder().CreatePHI(llvm::Type::getDoubleTy(Compiler::Internal::get_context()), 2, variable_name);
        variable->addIncoming(startv, preheader_block);

        // Save old variable if the loop variable overrides it
        llvm::Value* old_value = Compiler::Internal::get_named_values()[variable_name];
        Compiler::Internal::get_named_values()[variable_name] = variable;

        if (!body->codegen()) {
            return nullptr;
        }

        llvm::Value* step_value = nullptr;
        if (step) {
            step_value = step->codegen();
            if (!step_value) {
                return nullptr;
            }
        }
        else {
            step_value = llvm::ConstantFP::get(Compiler::Internal::get_context(), llvm::APFloat(1.0));
        }
        llvm::Value* next_variable = Compiler::Internal::get_builder().CreateFAdd(variable, step_value, "nextvariable");

        llvm::Value* end_condition = end->codegen();
        if (!end_condition) {
            return nullptr;
        }
        // Convert condition to a bool by comparing to 0.0
        end_condition = Compiler::Internal::get_builder().CreateFCmpONE(end_condition, llvm::ConstantFP::get(Compiler::Internal::get_context(), llvm::APFloat(0.0)), "loopcondition");

        //Evaluate if loop should exit
        llvm::BasicBlock* loop_end_block = Compiler::Internal::get_builder().GetInsertBlock();
        llvm::BasicBlock* after_loop_block = llvm::BasicBlock::Create(Compiler::Internal::get_context(), "afterloop", f);
        Compiler::Internal::get_builder().CreateCondBr(end_condition, loop_block, after_loop_block);
        Compiler::Internal::get_builder().SetInsertPoint(after_loop_block);

        variable->addIncoming(next_variable, loop_end_block);

        // Restore old variable if necessary
        if (old_value) {
            Compiler::Internal::get_named_values()[variable_name] = old_value;
        }
        else {
            Compiler::Internal::get_named_values().erase(variable_name);
        }

        // return 0
        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(Compiler::Internal::get_context()));
    }

}
