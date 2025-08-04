#include "ast/if_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> IfExpression::codegen() {
        // Push 'None' as the target type in order to let value expressions choose their default type
        Type::TargetTypeStack::push(Type::TypeToken::None);

        std::unique_ptr<ExpressionResult> condition_er = condition->codegen();
        if (!condition_er->is_valid()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in if condition");
            return ExpressionResult::create_invalid();
        }

        Type::TargetTypeStack::pop();

        // Manually check for casting because if the condition would be a CastExpression, 'if's would be cluttered with compile warnings
        if (condition_er->get_type() != Type::TypeToken::Bool) {
            condition_er->set_value(Type::cast(condition_er->get_value(), condition_er->get_type(), Type::TypeToken::Bool));
        }

        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();

        llvm::BasicBlock* if_block = llvm::BasicBlock::Create(Compiler::get_context(), "ifbranch", f);
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(Compiler::get_context(), "elsebranch", f);
        llvm::BasicBlock* after_block = llvm::BasicBlock::Create(Compiler::get_context(), "afterbranch", f);

        Compiler::get_builder().CreateCondBr(condition_er->get_value(), if_block, else_block);

        // Codegen if body
        Compiler::get_builder().SetInsertPoint(if_block);

        bool if_body_has_return = false;
        for (int i = 0; i < if_body.size(); i++) {
            std::unique_ptr<ExpressionResult> if_body_er = if_body[i]->codegen();
            if (!if_body_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in if body");
                return ExpressionResult::create_invalid();
            }
            if (if_body_er->is_return_statement() || if_body_er->forms_qualified_return()) {
                if (if_body.size() - (i + 1) > 0) {
                    log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": unreachable code in if body detected");
                }
                if_body_has_return = true;
                break;
            }
        }

        if (!if_body_has_return) {
            Compiler::get_builder().CreateBr(after_block);
        }

        // Codegen else body
        Compiler::get_builder().SetInsertPoint(else_block);

        bool else_body_has_return = false;
        for (int i = 0; i < else_body.size(); i++) {
            std::unique_ptr<ExpressionResult> else_body_er = else_body[i]->codegen();
            if (!else_body_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in else body");
                return ExpressionResult::create_invalid();
            }
            if (else_body_er->is_return_statement() || else_body_er->forms_qualified_return()) {
                if (else_body.size() - (i + 1) > 0) {
                    log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": unreachable code in else body detected");
                }
                else_body_has_return = true;
                break;
            }
        }

        if (!else_body_has_return) {
            Compiler::get_builder().CreateBr(after_block);
        }

        Compiler::get_builder().SetInsertPoint(after_block);

        unsigned int flags = ExpressionResultFlags::Valid;
        if (if_body_has_return && else_body_has_return) {
            flags |= ExpressionResultFlags::QualifiedReturn;
        }
        return ExpressionResult::create(nullptr, Type::TypeToken::None, flags);
    }

}
