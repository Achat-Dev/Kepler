// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "codegen/codegen_pass.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include "utils/string_pool.hpp"
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    std::unique_ptr<llvm::Module> CodegenPass::run() {
        forward_declare_prototypes(ast.top_level_nodes);
        codegen_nodes(ast.top_level_nodes);
        return std::move(module);
    }

    void CodegenPass::forward_declare_prototypes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(node.get());
                    codegen_forward_declaration(ext->prototype.get());
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(node.get());
                    codegen_forward_declaration(function->prototype.get());
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type on top level during prototype forward declaration");
            }
        }
    }

    void CodegenPass::codegen_forward_declaration(const Prototype* prototype) {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_NOT_NULLPTR(prototype->return_type);
        KPL_ASSERT_NOT_NULLPTR(prototype->symbol);
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison, "Prototype must not be poisoned for forward declaration");

        std::vector<llvm::Type*> parameter_types;
        for (const ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_NOT_NULLPTR(parameter_data.type);
            parameter_types.push_back(get_llvm_type(parameter_data.type, context));
        }

        const std::string_view prototype_name = StringPool::get().lookup(prototype->identifier_id);
        llvm::FunctionType* function_type = llvm::FunctionType::get(get_llvm_type(prototype->return_type, context), parameter_types, false);
        llvm::Function* function = llvm::Function::Create(function_type, get_llvm_linkage_type(prototype->linkage_type), prototype_name, *module);

#ifndef NDEBUG
        unsigned int index = 0;
        for (llvm::Argument& arg : function->args()) {
            const std::string_view parameter_name = StringPool::get().lookup(prototype->parameter_data[index].identifier_id);
            arg.setName(parameter_name);
        }
#endif

        KPL_ASSERT_THAT(!llvm_values.contains(prototype->symbol), "LLVM value for prototype symbol already exists");
        llvm_values[prototype->symbol] = function;
    }

    void CodegenPass::codegen_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            CodegenResult codegen_result = codegen_node(node.get());
            if (codegen_result.returns) {
                break;
            }
        }
    }

    CodegenResult CodegenPass::codegen_node(const ASTNode* node) {
        KPL_ASSERT_NOT_NULLPTR(node);

        switch (node->node_type) {
            case ASTNodeType::Poison:
                return {.llvm_value = nullptr, .returns = false};
            case ASTNodeType::Extern:
                return {.llvm_value = nullptr, .returns = false};
            case ASTNodeType::Function:
                codegen_function(static_cast<const Function*>(node));
                return {.llvm_value = nullptr, .returns = false};
            case ASTNodeType::Prototype:
                KPL_ASSERT_UNREACHABLE("Cannot codegen a prototype");
            case ASTNodeType::AssignmentStatement:
                return codegen_assignment_statement(static_cast<const AssignmentStatement*>(node));
            case ASTNodeType::ForStatement:
                return codegen_for_statement(static_cast<const ForStatement*>(node));
            case ASTNodeType::IfStatement:
                return codegen_if_statement(static_cast<const IfStatement*>(node));
            case ASTNodeType::ReturnStatement:
                return codegen_return_statement(static_cast<const ReturnStatement*>(node));
            case ASTNodeType::VariableDefinitionStatement:
                return codegen_variable_definition_statement(static_cast<const VariableDefinitionStatement*>(node));
            case ASTNodeType::BooleanLiteralExpression:
                return codegen_boolean_literal_expression(static_cast<const BooleanLiteralExpression*>(node));
            case ASTNodeType::FloatingPointLiteralExpression:
                return codegen_floating_point_literal_expression(static_cast<const FloatingPointLiteralExpression*>(node));
            case ASTNodeType::IntegerLiteralExpression:
                return codegen_integer_literal_expression(static_cast<const IntegerLiteralExpression*>(node));
            case ASTNodeType::StringLiteralExpression:
                return codegen_string_literal_expression(static_cast<const StringLiteralExpression*>(node));
            case ASTNodeType::BinaryExpression:
                return codegen_binary_expression(static_cast<const BinaryExpression*>(node));
            case ASTNodeType::CallExpression:
                return codegen_call_expression(static_cast<const CallExpression*>(node));
            case ASTNodeType::CastExpression:
                return codegen_cast_expression(static_cast<const CastExpression*>(node));
            case ASTNodeType::MathematicalNegationExpression:
                return codegen_mathematical_negation_expression(static_cast<const MathematicalNegationExpression*>(node));
            case ASTNodeType::VariableExpression:
                return codegen_variable_expression(static_cast<const VariableExpression*>(node));
        }
        KPL_ASSERT_UNREACHABLE("Missing codegen implementation for node type '{}'", node->node_type);
    }

    void CodegenPass::codegen_function(const Function* function) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_NOT_NULLPTR(function->prototype->symbol);
        KPL_ASSERT_THAT(llvm_values.contains(function->prototype->symbol), "LLVM Function must exist for codegening a function");

        // Create entry block
        llvm::Function* llvm_function = static_cast<llvm::Function*>(llvm_values[function->prototype->symbol]);
        llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(context, "entry", llvm_function);
        builder.SetInsertPoint(entry_block);

        // Create allocas for function parameters
        int index = 0;
        for (llvm::Argument& arg : llvm_function->args()) {
            llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_function, arg.getType(), function->prototype->identifier_id);
            builder.CreateStore(&arg, alloca);

            Symbol* parameter_symbol = function->prototype->parameter_data[index].symbol;
            KPL_ASSERT_THAT(!llvm_values.contains(parameter_symbol), "LLVM value for function parameter can't exist for codegening a function");
            llvm_values[parameter_symbol] = alloca;
            index++;
        }

        // Codegen the body
        codegen_nodes(function->body.nodes);

        // Create implicit return for void methods
        if (!function->body.contains_return && function->prototype->return_type == type_table.Builtins.void_type) {
            KPL_ASSERT_THAT(builder.GetInsertBlock()->getTerminator() == nullptr,
                "When body of void function doesn't contain a return statement, the generated ir isn't allowed to have a terminator after code generation");
            builder.CreateRetVoid();
        }

        // Right now the only unreachable blocks are created by for and if statements
        // The codegen methods for these two already remove unreachable blocks, so there is no need to call EliminateUnreachableBlocks
        // If it ever becomes necessary to call the method, this is the place to do so
        // llvm::EliminateUnreachableBlocks(*llvm_function);
        builder.ClearInsertionPoint();

        // Check ir for errors
        std::string error;
        llvm::raw_string_ostream raw_string_ostream(error);
        bool is_invalid_function = llvm::verifyFunction(*llvm_function, &raw_string_ostream);
        raw_string_ostream.flush();

        if (is_invalid_function) {
            log::error("Invalid llvm function ir:\n{}", error);
            llvm_function->print(llvm::errs());
            llvm_function->eraseFromParent();
        }
    }

    llvm::AllocaInst* CodegenPass::create_entry_block_alloca(llvm::Function* function, llvm::Type* type, StringId identifier_id) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_THAT(!function->empty(), "LLVM Function must have an entry block to create an entry block alloca");
        KPL_ASSERT_NOT_NULLPTR(type);
        llvm::IRBuilder<> tmp_builder(&function->getEntryBlock(), function->getEntryBlock().begin());
#ifndef NDEBUG
        const std::string_view identifier = StringPool::get().lookup(identifier_id);
        return tmp_builder.CreateAlloca(type, nullptr, identifier);
#else
        return tmp_builder.CreateAlloca(type);
#endif
    }

    CodegenResult CodegenPass::codegen_assignment_statement(const AssignmentStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "AssignmentStatement must not be poisoned for code generation");

        Symbol* variable_symbol = statement->variable_expression->symbol;
        KPL_ASSERT_NOT_NULLPTR(variable_symbol);
        KPL_ASSERT_THAT(llvm_values.contains(variable_symbol),
            "LLVM value for variable must exist for codegening an AssignmentStatement");

        // Don't codegen the VariableExpression because that would just unnecessarilly load it
        const CodegenResult codegen_result = codegen_node(statement->value_expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        builder.CreateStore(codegen_result.llvm_value, llvm_values[variable_symbol]);
        return {.llvm_value = nullptr, .returns = false};
    }

    CodegenResult CodegenPass::codegen_for_statement(const ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ForStatement must not be poisoned for code generation");

        const CodegenResult variable_cr = codegen_variable_definition_statement(statement->loop_variable_definition.get());
        KPL_ASSERT_NOT_NULLPTR(variable_cr.llvm_value);
        KPL_ASSERT_THAT(llvm::isa<llvm::AllocaInst>(variable_cr.llvm_value), "LLVM value of loop variable must be an AllocaInst");
        const CodegenResult end_cr = codegen_node(statement->end_value.get());
        KPL_ASSERT_NOT_NULLPTR(end_cr.llvm_value);

        // Codegen step value
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition->type);
        llvm::Type* variable_type = get_llvm_type(statement->loop_variable_definition->type, context);
        llvm::Value* step_value = nullptr;
        if (statement->step_value == nullptr) {
            // Step is implicit, so make it:
            // 1 if start <= end
            // -1 if start > end
            llvm::Value* variable_load = builder.CreateLoad(variable_type, variable_cr.llvm_value);
            llvm::Value* start_le_end = builder.CreateICmpSLE(variable_load, end_cr.llvm_value);
            step_value = builder.CreateSelect(start_le_end,
                llvm::ConstantInt::get(variable_type, 1),
                llvm::ConstantInt::getSigned(variable_type, -1));
        } else {
            const CodegenResult step_cr = codegen_node(statement->step_value.get());
            KPL_ASSERT_NOT_NULLPTR(step_cr.llvm_value);
            step_value = step_cr.llvm_value;
        }
        KPL_ASSERT_NOT_NULLPTR(step_value);

        // Prepare BasicBlocks
        llvm::Function* llvm_function = builder.GetInsertBlock()->getParent();
        KPL_ASSERT_NOT_NULLPTR(llvm_function);
        llvm::BasicBlock* header_block = llvm::BasicBlock::Create(context, "loop_header", llvm_function);
        llvm::BasicBlock* body_block = llvm::BasicBlock::Create(context, "loop_body", llvm_function);
        llvm::BasicBlock* increment_block = llvm::BasicBlock::Create(context, "loop_increment", llvm_function);
        llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "after_loop", llvm_function);
        builder.CreateBr(header_block);
        builder.SetInsertPoint(header_block);

        // Codegen header
        if (statement->body.contains_return) {
            builder.CreateBr(body_block);
        } else {
            llvm::Value* variable_load = builder.CreateLoad(variable_type, variable_cr.llvm_value);
            llvm::Value* loop_condition = builder.CreateICmpEQ(variable_load, end_cr.llvm_value);
            builder.CreateCondBr(loop_condition, after_block, body_block);
        }
        builder.SetInsertPoint(body_block);

        // Codegen body
        codegen_nodes(statement->body.nodes);
        if (statement->body.contains_return) {
            increment_block->eraseFromParent();
            after_block->eraseFromParent();
            return {.llvm_value = nullptr, .returns = true};
        }
        builder.CreateBr(increment_block);
        builder.SetInsertPoint(increment_block);

        // Codegen increment
        llvm::Value* variable_load = builder.CreateLoad(variable_type, variable_cr.llvm_value);
        llvm::Value* incremented_variable_value = create_add(variable_load, end_cr.llvm_value, statement->loop_variable_definition->type, builder);
        builder.CreateStore(incremented_variable_value, variable_cr.llvm_value);
        builder.CreateBr(header_block);

        builder.SetInsertPoint(after_block);
        return {.llvm_value = nullptr, .returns = false};
    }

    CodegenResult CodegenPass::codegen_if_statement(const IfStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "IfStatement must not be poisoned for code generation");
        const CodegenResult condition_cr = codegen_node(statement->condition.get());
        KPL_ASSERT_NOT_NULLPTR(condition_cr.llvm_value);

        // Prepare BasicBlocks
        llvm::Function* llvm_function = builder.GetInsertBlock()->getParent();
        KPL_ASSERT_NOT_NULLPTR(llvm_function);
        llvm::BasicBlock* if_block = llvm::BasicBlock::Create(context, "if_body", llvm_function);
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(context, "else_body", llvm_function);
        llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "after_if", llvm_function);
        builder.CreateCondBr(condition_cr.llvm_value, if_block, else_block);

        // Codegen 'if' body
        builder.SetInsertPoint(if_block);
        codegen_nodes(statement->if_body.nodes);
        if (!statement->if_body.contains_return) {
            builder.CreateBr(after_block);
        }
        builder.SetInsertPoint(else_block);

        // Codegen 'else' body
        codegen_nodes(statement->else_body.nodes);
        if (!statement->else_body.contains_return) {
            builder.CreateBr(after_block);
        }

        const bool returns = statement->if_body.contains_return && statement->else_body.contains_return;
        if (returns) {
            after_block->eraseFromParent();
        } else {
            builder.SetInsertPoint(after_block);
        }
        return {.llvm_value = nullptr, .returns = returns};
    }

    CodegenResult CodegenPass::codegen_return_statement(const ReturnStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ReturnStatement must not be poisoned for code generation");
        if (statement->expression == nullptr) {
            llvm::ReturnInst* return_inst = builder.CreateRetVoid();
            return {.llvm_value = return_inst, .returns = true};
        } else {
            const CodegenResult codegen_result = codegen_node(statement->expression.get());
            KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
            llvm::ReturnInst* return_inst = builder.CreateRet(codegen_result.llvm_value);
            return {.llvm_value = return_inst, .returns = true};
        }
    }

    CodegenResult CodegenPass::codegen_variable_definition_statement(const VariableDefinitionStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_NOT_NULLPTR(statement->type);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "VariableDefinitionStatement must not be poisoned for code generation");
        llvm::Function* llvm_function = builder.GetInsertBlock()->getParent();
        KPL_ASSERT_NOT_NULLPTR(llvm_function);
        llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_function, get_llvm_type(statement->type, context), statement->identifier_id);

        Symbol* variable_symbol = statement->assignment_statement->variable_expression->symbol;
        KPL_ASSERT_NOT_NULLPTR(variable_symbol);
        KPL_ASSERT_THAT(!llvm_values.contains(variable_symbol), "LLVM value for variable can't exist for codegening a VariableDefinitionStatement");
        llvm_values.emplace(variable_symbol, alloca);
        codegen_assignment_statement(statement->assignment_statement.get());
        return {.llvm_value = alloca, .returns = false};
    }

    CodegenResult CodegenPass::codegen_boolean_literal_expression(const BooleanLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BooleanLiteralExpression must not be poisoned for code generation");
        return {.llvm_value = llvm::ConstantInt::getBool(context, expression->value), .returns = false};
    }

    CodegenResult CodegenPass::codegen_floating_point_literal_expression(const FloatingPointLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "FloatingPointLiteralExpression must not be poisoned for code generation");
        llvm::Type* llvm_type = get_llvm_type(expression->target_type, context);
        return {.llvm_value = llvm::ConstantFP::get(llvm_type, expression->value), .returns = false};
    }

    CodegenResult CodegenPass::codegen_integer_literal_expression(const IntegerLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "IntegerLiteralExpression must not be poisoned for code generation");
        if (is_floating_point_type(expression->target_type)) {
            return {.llvm_value = llvm::ConstantFP::get(get_llvm_type(expression->target_type, context), expression->value), .returns = false};
        }
        return {.llvm_value = llvm::ConstantInt::getSigned(get_llvm_type(expression->target_type, context), expression->value), .returns = false};
    }

    CodegenResult CodegenPass::codegen_string_literal_expression(const StringLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "StringLiteralExpression must not be poisoned for code generation");

        const std::string_view string_value = StringPool::get().lookup(expression->value);
        // Constant array that holds the string data (null terminated)
        llvm::Constant* data = llvm::ConstantDataArray::getString(context, string_value);
        // Global pointer that points to the constant array
        // This is owned by the llvm module and doesn't have to be freed manually
        llvm::GlobalVariable* global_variable = new llvm::GlobalVariable(*module, data->getType(), true, llvm::GlobalValue::PrivateLinkage, data);

        // Optimisation: tell llvm that the pointer is never going to be compared
        // (only the value of the string is going to be compared, never the reference to the string)
        global_variable->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        // Create i8* to the first element of the constant array
        llvm::Constant* zero = llvm::ConstantInt::get(get_llvm_type(type_table.Builtins.i32_type, context), 0);
        llvm::Constant* indices[] = {zero, zero};
        llvm::Constant* value = llvm::ConstantExpr::getInBoundsGetElementPtr(data->getType(), global_variable, indices);

        // If strings should be character arrays, the global_variable could be returned (that copies the data)
        return {.llvm_value = value, .returns = false};
    }

    CodegenResult CodegenPass::codegen_binary_expression(const BinaryExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BinaryExpression must not be poisoned for code generation");

        const CodegenResult lhs_cr = codegen_node(expression->lhs.get());
        KPL_ASSERT_NOT_NULLPTR(lhs_cr.llvm_value);
        const CodegenResult rhs_cr = codegen_node(expression->rhs.get());
        KPL_ASSERT_NOT_NULLPTR(rhs_cr.llvm_value);

        switch (expression->operator_type) {
            case OperatorType::Plus:
                return {.llvm_value = create_add(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::Minus:
                return {.llvm_value = create_sub(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::Multiplication:
                return {.llvm_value = create_mul(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::Division:
                return {.llvm_value = create_div(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::LessThan:
                return {.llvm_value = create_less_than(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::GreaterThan:
                return {.llvm_value = create_greater_than(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::Equals:
                return {.llvm_value = create_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::NotEquals:
                return {.llvm_value = create_not_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::LessEquals:
                return {.llvm_value = create_less_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
            case OperatorType::GreaterEquals:
                return {.llvm_value = create_greater_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, expression->target_type, builder), .returns = false};
        }

        KPL_ASSERT_UNREACHABLE("Missing codegen implementation for operator type '{}'", expression->operator_type);
    }

    CodegenResult CodegenPass::codegen_call_expression(const CallExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->symbol);
        KPL_ASSERT_THAT(llvm_values.contains(expression->symbol), "LLVM value for function must exist for codegening a CallExpression");

        llvm::Function* llvm_function = static_cast<llvm::Function*>(llvm_values[expression->symbol]);
        KPL_ASSERT_THAT(llvm_function->arg_size() == expression->args.size(),
            "Parameter count of LLVM function and CallExpression have to be the same, received {} and {}",
            llvm_function->arg_size(),
            expression->args.size());

        std::vector<llvm::Value*> arg_values;
        for (const std::unique_ptr<Expression>& arg : expression->args) {
            const CodegenResult codegen_result = codegen_node(arg.get());
            KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
            arg_values.push_back(codegen_result.llvm_value);
        }

        llvm::Value* value = nullptr;
        if (expression->symbol->type == type_table.Builtins.void_type) {
            value = builder.CreateCall(llvm_function, std::move(arg_values));
        } else {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            value = builder.CreateCall(llvm_function, std::move(arg_values), "call_" + std::string(identifier));
        }
        return {.llvm_value = value, .returns = false};
    }

    CodegenResult CodegenPass::codegen_cast_expression(const CastExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_NOT_NULLPTR(expression->original_type);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CastExpression must not be poisoned for code generation");

        const CodegenResult codegen_result = codegen_node(expression->expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        if (expression->original_type == expression->target_type) {
            // Redundant cast, so just return the original value
            return {.llvm_value = codegen_result.llvm_value, .returns = false};
        }
        return {.llvm_value = create_cast(codegen_result.llvm_value, expression->original_type, expression->target_type, context, builder), .returns = false};
    }

    CodegenResult CodegenPass::codegen_mathematical_negation_expression(const MathematicalNegationExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "MathematicalNegationExpression must not be poisoned for code generation");

        const CodegenResult codegen_result = codegen_node(expression->expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        if (is_integer_type(expression->target_type)) {
            return {.llvm_value = builder.CreateNeg(codegen_result.llvm_value), .returns = false};
        } else if (is_floating_point_type(expression->target_type)) {
            return {.llvm_value = builder.CreateFNeg(codegen_result.llvm_value), .returns = false};
        }
        KPL_ASSERT_UNREACHABLE("Missing create mathematical negation implementation for type '{}'", *expression->target_type);
    }

    CodegenResult CodegenPass::codegen_variable_expression(const VariableExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "VariableExpression must not be poisoned for code generation");
        Symbol* symbol = expression->symbol;
        KPL_ASSERT_NOT_NULLPTR(symbol);
        KPL_ASSERT_NOT_NULLPTR(symbol->type);
        KPL_ASSERT_THAT(llvm_values.contains(symbol), "LLVM value for variable must exist for codegening a VariableExpression");
        KPL_ASSERT_THAT(llvm::isa<llvm::AllocaInst>(llvm_values[symbol]), "LLVM value for variable must be an AllocaInst");
#ifndef NDEBUG
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        llvm::Value* value = builder.CreateLoad(get_llvm_type(symbol->type, context), llvm_values[symbol], identifier);
#else
        llvm::Value* value = builder.CreateLoad(get_llvm_type(symbol->type, context), llvm_values[symbol]);
#endif
        return {.llvm_value = value, .returns = false};
    }

}
