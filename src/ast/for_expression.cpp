#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "../utils.hpp"
#include "for_expression.hpp"

namespace Kepler::AST {

    llvm::Value* ForExpression::codegen() {
        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();
        llvm::AllocaInst* alloca = create_entry_block_alloca(f, variable_name);

        llvm::Value* startv = start->codegen();
        if (!startv) {
            return nullptr;
        }

        Compiler::get_builder().CreateStore(startv, alloca);

        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "loop", f);
        Compiler::get_builder().CreateBr(loop_block);
        Compiler::get_builder().SetInsertPoint(loop_block);

        // Save old variable if the loop variable overrides it
        llvm::AllocaInst* old_value = Compiler::get_named_values()[variable_name];
        Compiler::get_named_values()[variable_name] = alloca;

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
            step_value = llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(1.0));
        }

        llvm::Value* end_condition = end->codegen();
        if (!end_condition) {
            return nullptr;
        }
        // Convert condition to a bool by comparing to 0.0
        end_condition = Compiler::get_builder().CreateFCmpONE(end_condition, llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(0.0)), "loopcondition");

        llvm::Value* current_variable = Compiler::get_builder().CreateLoad(alloca->getAllocatedType(), alloca, variable_name.c_str());
        llvm::Value* next_variable = Compiler::get_builder().CreateFAdd(current_variable, step_value, "nextvariable");
        Compiler::get_builder().CreateStore(next_variable, alloca);

        //Evaluate if loop should exit
        llvm::BasicBlock* after_loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "afterloop", f);
        Compiler::get_builder().CreateCondBr(end_condition, loop_block, after_loop_block);
        Compiler::get_builder().SetInsertPoint(after_loop_block);

        // Restore old variable if necessary
        if (old_value) {
            Compiler::get_named_values()[variable_name] = old_value;
        }
        else {
            Compiler::get_named_values().erase(variable_name);
        }

        // return 0
        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(Compiler::get_context()));
    }

}
