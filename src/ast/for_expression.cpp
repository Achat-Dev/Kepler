#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <optional>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../types/target_type_stack.hpp"
#include "../variables/variable_data.hpp"
#include "../variables/local_variables.hpp"
#include "expression.hpp"
#include "expression_result.hpp"
#include "for_expression.hpp"

namespace Kepler::AST {

    static llvm::Value* codegen_end_condition(llvm::Value* select_condition, llvm::AllocaInst* alloca, llvm::Value* endv, const char* variable_name) {
        return Compiler::get_builder().CreateSelect(select_condition,
            Compiler::get_builder().CreateFCmpULT(Compiler::get_builder().CreateLoad(alloca->getAllocatedType(), alloca, variable_name), endv, "loopconditionlt"),
            Compiler::get_builder().CreateFCmpUGT(Compiler::get_builder().CreateLoad(alloca->getAllocatedType(), alloca, variable_name), endv, "loopconditiongt")
        );
    }

    std::unique_ptr<ExpressionResult> ForExpression::codegen() {
        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();

        // Save old variable if the loop variable overrides it
        std::string variable_name = start->get_name();
        std::optional<LocalVariables::VariableData> old_variable = LocalVariables::get(variable_name);

        // Codegen start value
        std::unique_ptr<ExpressionResult> start_er = start->codegen();
        if (!start_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' start value");
            return ExpressionResult::create_invalid();
        }
        llvm::AllocaInst* alloca = LocalVariables::get(variable_name)->variable;

        Type::TargetTypeStack::push(start_er->get_type());

        // Codegen the end condition
        // This needs to be codegened in the entry block of the function in order to determine if the loop should be entered at all
        std::unique_ptr<ExpressionResult> end_er = end->codegen();
        if (!end_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' end value");
            return ExpressionResult::create_invalid();
        }
        llvm::Value* end_select_condition = Compiler::get_builder().CreateFCmpULE(start_er->get_value(), end_er->get_value());
        llvm::Value* start_condition = codegen_end_condition(end_select_condition, alloca, end_er->get_value(), variable_name.c_str());

        // Don't create a terminator for the entry block yet because that depends on if the body is a qualified return
        llvm::BasicBlock* entry_block = Compiler::get_builder().GetInsertBlock();
        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "loop", f);
        Compiler::get_builder().SetInsertPoint(loop_block);

        // Pop target type before codegening body
        Type::TargetTypeStack::pop();

        // Codegen body
        for (int i = 0; i < body.size(); i++) {
            std::unique_ptr<ExpressionResult> body_er = body[i]->codegen();
            if (!body_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' body");
                return ExpressionResult::create_invalid();
            }
            if (body_er->is_return_statement() || body_er->forms_qualified_return()) {
                log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": return in 'for' body detected. This will always be executed even if the loop condition is false at start. Consider removing the for loop");

                // Create the terminator for the entry block -> branch to the loop_block
                Compiler::get_builder().SetInsertPoint(entry_block);
                Compiler::get_builder().CreateBr(loop_block);
                Compiler::get_builder().SetInsertPoint(loop_block);

                return ExpressionResult::create(nullptr, Type::TypeToken::None, ExpressionResultFlags::Valid | ExpressionResultFlags::QualifiedReturn);
            }
        }

        Type::TargetTypeStack::push(start_er->get_type());

        // Codegen step
        llvm::Value* step_v;
        if (step) {
            std::unique_ptr<ExpressionResult> step_er = step->codegen();
            if (!step_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in 'for' step value");
                return ExpressionResult::create_invalid();
            }
            step_v = step_er->get_value();
        }
        else {
            // Step is implicit, so we need to dynamically decide if it should be 1 or -1
            step_v = Compiler::get_builder().CreateSelect(end_select_condition,
                llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(1.0)),
                llvm::ConstantFP::get(Compiler::get_context(), llvm::APFloat(-1.0))
            );
        }

        Type::TargetTypeStack::pop();

        // Calculate loop variable for next iteration
        llvm::Value* current_variable = Compiler::get_builder().CreateLoad(alloca->getAllocatedType(), alloca, variable_name.c_str());
        llvm::Value* next_variable = Compiler::get_builder().CreateFAdd(current_variable, step_v, "nextvariable");
        Compiler::get_builder().CreateStore(next_variable, alloca);

        // Codegen loop end condition in the body to check if the loop should be exited
        llvm::Value* end_condition = codegen_end_condition(end_select_condition, alloca, end_er->get_value(), variable_name.c_str());

        //Evaluate if loop should exit
        llvm::BasicBlock* after_loop_block = llvm::BasicBlock::Create(Compiler::get_context(), "afterloop", f);
        Compiler::get_builder().CreateCondBr(end_condition, loop_block, after_loop_block);

        // Create the terminator for the entry block -> conditional branch to either the loop_block or after_loop_block
        Compiler::get_builder().SetInsertPoint(entry_block);
        Compiler::get_builder().CreateCondBr(start_condition, loop_block, after_loop_block);

        Compiler::get_builder().SetInsertPoint(after_loop_block);

        // Restore old variable if necessary
        if (old_variable) {
            LocalVariables::update(variable_name, *old_variable);
        }
        else {
            LocalVariables::erase(variable_name);
        }

        return ExpressionResult::create(nullptr, Type::TypeToken::None, ExpressionResultFlags::Valid);
    }

}
