// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "codegen/codegen_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
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
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "ast/struct.hpp"
#include "codegen/optimizer.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include "utils/string_pool.hpp"
#include "llvm/IR/Instruction.h"
#include <algorithm>
#include <cstdint>
#include <format>
#include <llvm/ADT/APInt.h>
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
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    CodegenPass::CodegenPass(DiagnosticSink& diagnostic_sink,
        SymbolTable& symbol_table,
        TypeTable& type_table,
        llvm::LLVMContext& context,
        llvm::TargetMachine* target_machine,
        OptimizationLevel optimization_level)
        : diagnostic_sink(diagnostic_sink),
          symbol_table(symbol_table),
          type_table(type_table),
          target_machine(target_machine),
          optimization_level(optimization_level),
          context(context),
          builder(context) {
        llvm_types.emplace(type_table.Builtins.void_type_id, llvm::Type::getVoidTy(context));
        llvm_types.emplace(type_table.Builtins.bool_type_id, llvm::Type::getInt1Ty(context));
        // A string is internally represented as an immutable array of i8
        // However, to get the llvm::Type* of that, the length of the array is needed
        // That's why the type of a string is an i8* (since llvm uses opaque pointers, the pointer is not explicitly typed)
        llvm_types.emplace(type_table.Builtins.string_type_id, llvm::PointerType::get(context, 0));
        llvm_types.emplace(type_table.Builtins.i8_type_id, llvm::Type::getInt8Ty(context));
        llvm_types.emplace(type_table.Builtins.i16_type_id, llvm::Type::getInt16Ty(context));
        llvm_types.emplace(type_table.Builtins.i32_type_id, llvm::Type::getInt32Ty(context));
        llvm_types.emplace(type_table.Builtins.i64_type_id, llvm::Type::getInt64Ty(context));
        llvm_types.emplace(type_table.Builtins.u8_type_id, llvm::Type::getInt8Ty(context));
        llvm_types.emplace(type_table.Builtins.u16_type_id, llvm::Type::getInt16Ty(context));
        llvm_types.emplace(type_table.Builtins.u32_type_id, llvm::Type::getInt32Ty(context));
        llvm_types.emplace(type_table.Builtins.u64_type_id, llvm::Type::getInt64Ty(context));
        llvm_types.emplace(type_table.Builtins.f32_type_id, llvm::Type::getFloatTy(context));
        llvm_types.emplace(type_table.Builtins.f64_type_id, llvm::Type::getDoubleTy(context));
    }

    std::optional<std::vector<std::unique_ptr<llvm::Module>>> CodegenPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        std::unordered_map<ModuleId, std::unique_ptr<llvm::Module>> llvm_modules;
        // Forward declare prototypes
        for (const AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            const auto it = llvm_modules.find(ast.module_statement->module_id);
            if (it != llvm_modules.end()) {
                current_llvm_module = it->second.get();
            } else {
                std::string module_identifier = get_full_module_identifier(ast.module_statement->module_path);
                std::replace(module_identifier.begin(), module_identifier.end(), ':', '_');
                const auto [it, emplaced] = llvm_modules.emplace(ast.module_statement->module_id,
                    std::make_unique<llvm::Module>(module_identifier, context));
                KPL_ASSERT_THAT(emplaced);
                current_llvm_module = it->second.get();
            }
            forward_declare_structs(ast.struct_nodes);
            forward_declare_prototypes(ast.top_level_nodes);
        }
        current_llvm_module = nullptr;

        // Code generation
        for (const AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_THAT(llvm_modules.contains(ast.module_statement->module_id));
            current_llvm_module = llvm_modules[ast.module_statement->module_id].get();
            codegen_struct_bodies(ast.struct_nodes);
            codegen_nodes(ast.top_level_nodes);
            current_llvm_module->print(llvm::outs(), nullptr);
        }
        current_llvm_module = nullptr;

        // IR checking and optimization
        for (auto& [identifier_id, module] : llvm_modules) {
            KPL_ASSERT_NOT_NULLPTR(module);
            // Check ir for errors
            std::string error;
            llvm::raw_string_ostream raw_string_ostream(error);
            bool is_invalid_function = llvm::verifyModule(*module, &raw_string_ostream);
            raw_string_ostream.flush();
            if (is_invalid_function) {
                log::error("Invalid llvm function ir:\n{}", error);
                module->print(llvm::errs(), nullptr);
                return std::nullopt;
            }

            // Optimize module
            // It's important for optimization to set the data layout before running the optimizer
            KPL_ASSERT_NOT_NULLPTR(target_machine);
            module->setTargetTriple(target_machine->getTargetTriple());
            module->setDataLayout(target_machine->createDataLayout());
            optimize_module(module, optimization_level);
        }

        std::vector<std::unique_ptr<llvm::Module>> result;
        result.reserve(llvm_modules.size());
        for (auto& [identifier_id, module] : llvm_modules) {
            result.push_back(std::move(module));
        }
        llvm_modules.clear();
        return result;
    }

    void CodegenPass::open_scope() {
        symbol_id_scopes.push_back({});
    }

    void CodegenPass::close_scope() {
        KPL_ASSERT_THAT(!symbol_id_scopes.empty());
        const std::vector<SymbolId>& symbol_ids_to_remove = symbol_id_scopes.back();
        for (SymbolId symbol_id : symbol_ids_to_remove) {
            KPL_ASSERT_THAT(symbol_id != SymbolId::invalid());
            llvm_values.erase(symbol_id);
        }
        symbol_id_scopes.pop_back();
    }

    StringId CodegenPass::create_mangled_identifier(StringId identifier_id) const {
        KPL_ASSERT_NOT_NULLPTR(current_llvm_module);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const std::string& module_identifier = current_llvm_module->getModuleIdentifier();
        KPL_ASSERT_THAT(!module_identifier.empty());
        const std::string_view identifier = StringPool::get().lookup(identifier_id);
        KPL_ASSERT_THAT(!identifier.empty());
        const std::string mangled_name = std::format("3kpl{}{}{}{}", module_identifier.size(), module_identifier, identifier.size(), identifier);
        return StringPool::get().store(mangled_name);
    }

    bool CodegenPass::is_main_method(const Prototype* prototype) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(prototype->identifier_id != StringId::invalid());
        bool is_named_main = StringPool::get().lookup(prototype->identifier_id) == "main";
        bool returns_i32 = prototype->return_type_id == type_table.Builtins.i32_type_id;
        bool has_no_parameters = prototype->parameter_data.size() == 0;
        return is_named_main && returns_i32 && has_no_parameters;
    }

    llvm::Type* CodegenPass::get_llvm_type(TypeId type_id) {
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        KPL_ASSERT_THAT(type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(llvm_types.contains(type_id));
        return llvm_types[type_id];
    }

    void CodegenPass::forward_declare_structs(const std::vector<std::unique_ptr<Struct>>& struct_nodes) {
        for (const std::unique_ptr<Struct>& struct_node : struct_nodes) {
            // TODO (fix): Don't mangle name if it's an extern once extern structs are implemented
            KPL_ASSERT_NOT_NULLPTR(struct_node);
            KPL_ASSERT_THAT(struct_node->identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(struct_node->type_id != TypeId::invalid());
            const StringId mangled_identifier_id = create_mangled_identifier(struct_node->identifier_id);
            const Type* type = type_table.lookup(struct_node->type_id);
            llvm::StructType* llvm_type = llvm::StructType::create(context, StringPool::get().lookup(mangled_identifier_id));
            KPL_ASSERT_THAT(!llvm_types.contains(struct_node->type_id));
            const auto [it, emplaced] = llvm_types.emplace(struct_node->type_id, llvm_type);
            KPL_ASSERT_THAT(emplaced);
        }
    }

    void CodegenPass::forward_declare_prototypes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(ext->prototype);
                    forward_declare_prototype(ext->prototype.get(), ext->linkage_type, true);
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(function->prototype);
                    forward_declare_prototype(function->prototype.get(), function->linkage_type, false);
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type on top level during prototype forward declaration");
            }
        }
    }

    void CodegenPass::forward_declare_prototype(const Prototype* prototype, LinkageType linkage_type, bool is_extern) {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(prototype->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(prototype->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);

        std::vector<llvm::Type*> parameter_types;
        for (const ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type_id != TypeId::invalid());
            parameter_types.push_back(get_llvm_type(parameter_data.type_id));
        }

        StringId identifier_id = prototype->identifier_id;
        if (is_main_method(prototype)) {
            // No name mangling, force external linkage
            if (main_method_found) {
                diagnostic_sink.report(DiagnosticCode::MultipleMainMethods,
                    "'main' method is already defined and can only be defined once",
                    prototype->identifier_source_location);
            }
            linkage_type = LinkageType::External;
            main_method_found = true;
        } else {
            // Only mangle name if it's not an extern
            if (!is_extern) {
                identifier_id = create_mangled_identifier(prototype->identifier_id);
                Symbol* symbol = symbol_table.lookup(prototype->symbol_id);
                symbol->mangled_identifier_id = identifier_id;
            }
        }

        const std::string_view prototype_name = StringPool::get().lookup(identifier_id);
        llvm::FunctionType* function_type = llvm::FunctionType::get(get_llvm_type(prototype->return_type_id), parameter_types, prototype->is_variadic);
        llvm::Function* function = llvm::Function::Create(function_type, get_llvm_linkage_type(linkage_type), prototype_name, *current_llvm_module);

#ifndef NDEBUG
        unsigned int index = 0;
        for (llvm::Argument& arg : function->args()) {
            const std::string_view parameter_name = StringPool::get().lookup(prototype->parameter_data[index].identifier_id);
            arg.setName(parameter_name);
        }
#endif

        KPL_ASSERT_THAT(!llvm_values.contains(prototype->symbol_id));
        llvm_values[prototype->symbol_id] = function;
    }

    void CodegenPass::codegen_struct_bodies(const std::vector<std::unique_ptr<Struct>>& struct_nodes) {
        for (const std::unique_ptr<Struct>& struct_node : struct_nodes) {
            // TODO (fix): Structs can currently only have builtin types as members
            KPL_ASSERT_NOT_NULLPTR(struct_node);
            std::vector<llvm::Type*> member_types;
            member_types.reserve(struct_node->members.size());
            for (const StructNodeMember& member : struct_node->members) {
                KPL_ASSERT_THAT(member.type_id != TypeId::invalid());
                member_types.push_back(get_llvm_type(member.type_id));
            }

            KPL_ASSERT_THAT(struct_node->type_id != TypeId::invalid());
            llvm::Type* llvm_type = get_llvm_type(struct_node->type_id);
            KPL_ASSERT_THAT(llvm::isa<llvm::StructType>(llvm_type));
            llvm::StructType* llvm_struct_type = llvm::cast<llvm::StructType>(llvm_type);
            llvm_struct_type->setBody(member_types, false);
        }
    }

    void CodegenPass::codegen_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
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
            case ASTNodeType::Struct:
            case ASTNodeType::Prototype:
            case ASTNodeType::ImportStatement:
            case ASTNodeType::ModuleStatement:
                KPL_ASSERT_UNREACHABLE("Cannot codegen a node of type '{}' as part of the top level nodes", node->node_type);

            case ASTNodeType::Extern:
                return {.llvm_value = nullptr, .returns = false};
            case ASTNodeType::Function:
                codegen_function(static_cast<const Function*>(node));
                return {.llvm_value = nullptr, .returns = false};
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
            case ASTNodeType::ObjectInitializerExpression:
                return codegen_object_initializer_expression(static_cast<const ObjectInitializerExpression*>(node));
            case ASTNodeType::VariableExpression:
                return codegen_variable_expression(static_cast<const VariableExpression*>(node));
        }
        KPL_ASSERT_UNREACHABLE("Missing codegen implementation for node type '{}'", node->node_type);
    }

    void CodegenPass::codegen_function(const Function* function) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->prototype->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(function->prototype->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(llvm_values.contains(function->prototype->symbol_id));

        // Create entry block
        llvm::Function* llvm_function = static_cast<llvm::Function*>(llvm_values[function->prototype->symbol_id]);
        llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(context, "entry", llvm_function);
        builder.SetInsertPoint(entry_block);

        // Create allocas for function parameters
        open_scope();
        int index = 0;
        for (llvm::Argument& arg : llvm_function->args()) {
            llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_function, arg.getType(), function->prototype->identifier_id);
            builder.CreateStore(&arg, alloca);

            SymbolId parameter_symbol_id = function->prototype->parameter_data[index].symbol_id;
            KPL_ASSERT_THAT(parameter_symbol_id != SymbolId::invalid());
            KPL_ASSERT_THAT(!llvm_values.contains(parameter_symbol_id));
            llvm_values[parameter_symbol_id] = alloca;
            index++;
        }

        // Codegen the body
        codegen_nodes(function->body.nodes);
        close_scope();

        // Create implicit return for void methods
        if (!function->body.contains_return && function->prototype->return_type_id == type_table.Builtins.void_type_id) {
            const llvm::Instruction* terminator = builder.GetInsertBlock()->getTerminator();
            if (terminator != nullptr) {
                KPL_ASSERT_THAT(!terminator->isTerminator(),
                    "If body of void function doesn't contain a return statement, the generated ir isn't allowed to have a terminator after code generation");
            }
            builder.CreateRetVoid();
        }

        // Right now the only unreachable blocks are created by for and if statements
        // The codegen methods for these two already remove unreachable blocks, so there is no need to call EliminateUnreachableBlocks
        // If it ever becomes necessary to call the method, this is the place to do so
        // llvm::EliminateUnreachableBlocks(*llvm_function);
        builder.ClearInsertionPoint();
    }

    llvm::AllocaInst* CodegenPass::create_entry_block_alloca(llvm::Function* function, llvm::Type* type, StringId identifier_id) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_THAT(!function->empty(), "LLVM Function must have an entry block to create an entry block alloca");
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
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
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        SymbolId variable_symbol_id = statement->variable_expression->symbol_id;
        KPL_ASSERT_THAT(variable_symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(llvm_values.contains(variable_symbol_id));

        // Don't codegen the VariableExpression because that would just unnecessarilly load it
        const CodegenResult codegen_result = codegen_node(statement->value_expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        builder.CreateStore(codegen_result.llvm_value, llvm_values[variable_symbol_id]);
        return {.llvm_value = nullptr, .returns = false};
    }

    CodegenResult CodegenPass::codegen_for_statement(const ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        open_scope();
        const CodegenResult variable_cr = codegen_variable_definition_statement(statement->loop_variable_definition.get());
        KPL_ASSERT_NOT_NULLPTR(variable_cr.llvm_value);
        KPL_ASSERT_THAT(llvm::isa<llvm::AllocaInst>(variable_cr.llvm_value));
        const CodegenResult end_cr = codegen_node(statement->end_value.get());
        KPL_ASSERT_NOT_NULLPTR(end_cr.llvm_value);

        // Codegen step value
        const TypeId loop_variable_type_id = statement->loop_variable_definition->type_id;
        KPL_ASSERT_THAT(loop_variable_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(loop_variable_type_id != type_table.Builtins.unknown_type_id);
        const Type* loop_variable_type = type_table.lookup(loop_variable_type_id);
        llvm::Type* variable_type = get_llvm_type(loop_variable_type_id);
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
            close_scope();
            return {.llvm_value = nullptr, .returns = true};
        }
        builder.CreateBr(increment_block);
        builder.SetInsertPoint(increment_block);

        // Codegen increment
        llvm::Value* variable_load = builder.CreateLoad(variable_type, variable_cr.llvm_value);
        llvm::Value* incremented_variable_value = create_add(variable_load, step_value, loop_variable_type, builder);
        builder.CreateStore(incremented_variable_value, variable_cr.llvm_value);
        builder.CreateBr(header_block);

        builder.SetInsertPoint(after_block);
        close_scope();
        return {.llvm_value = nullptr, .returns = false};
    }

    CodegenResult CodegenPass::codegen_if_statement(const IfStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
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
        open_scope();
        builder.SetInsertPoint(if_block);
        codegen_nodes(statement->if_body.nodes);
        if (!statement->if_body.contains_return) {
            builder.CreateBr(after_block);
        }
        builder.SetInsertPoint(else_block);
        close_scope();

        // Codegen 'else' body
        open_scope();
        codegen_nodes(statement->else_body.nodes);
        if (!statement->else_body.contains_return) {
            builder.CreateBr(after_block);
        }
        close_scope();

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
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
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
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement->variable_expression);
        KPL_ASSERT_THAT(statement->type_id != TypeId::invalid());
        KPL_ASSERT_THAT(statement->type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(statement->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        llvm::Function* llvm_function = builder.GetInsertBlock()->getParent();
        KPL_ASSERT_NOT_NULLPTR(llvm_function);
        llvm::AllocaInst* alloca = create_entry_block_alloca(llvm_function, get_llvm_type(statement->type_id), statement->identifier_id);

        SymbolId variable_symbol_id = statement->assignment_statement->variable_expression->symbol_id;
        KPL_ASSERT_THAT(variable_symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(!llvm_values.contains(variable_symbol_id));
        llvm_values.emplace(variable_symbol_id, alloca);
        symbol_id_scopes.back().push_back(variable_symbol_id);
        codegen_assignment_statement(statement->assignment_statement.get());
        return {.llvm_value = alloca, .returns = false};
    }

    CodegenResult CodegenPass::codegen_boolean_literal_expression(const BooleanLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        return {.llvm_value = llvm::ConstantInt::getBool(context, expression->value), .returns = false};
    }

    CodegenResult CodegenPass::codegen_floating_point_literal_expression(const FloatingPointLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->target_type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        llvm::Type* llvm_type = get_llvm_type(expression->target_type_id);
        return {.llvm_value = llvm::ConstantFP::get(llvm_type, expression->value), .returns = false};
    }

    CodegenResult CodegenPass::codegen_integer_literal_expression(const IntegerLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->value_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        constexpr const uint8_t radix = 10; // This is basically the base for the llvm values -> base 10
        const std::string_view literal_string = StringPool::get().lookup(expression->value_id);
        KPL_ASSERT_THAT(!literal_string.empty());
        const Type* target_type = type_table.lookup(expression->target_type_id);
        if (is_integer_type(target_type)) {
            // Both signed and unsigned integers use llvm unsigned representation
            // This is because only the bit pattern counts: negative values are created through negation expressions,
            // which handle the negative values
            const uint32_t type_bitwidth = get_integer_bitwidth(target_type);
            const llvm::APInt llvm_value(type_bitwidth, literal_string, radix);
            return {.llvm_value = llvm::ConstantInt::get(get_llvm_type(expression->target_type_id), llvm_value), .returns = false};
        } else if (is_floating_point_type(target_type)) {
            const double value = std::stod(std::string(literal_string));
            return {.llvm_value = llvm::ConstantFP::get(get_llvm_type(expression->target_type_id), value), .returns = false};
        }
        KPL_ASSERT_UNREACHABLE("Target type of IntegerLiteralExpression must be either a signed integer, an unsigned integer or a floating point type");
    }

    CodegenResult CodegenPass::codegen_string_literal_expression(const StringLiteralExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->value_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        const std::string_view string_value = StringPool::get().lookup(expression->value_id);
        KPL_ASSERT_THAT(!string_value.empty());
        // Constant array that holds the string data (null terminated)
        llvm::Constant* data = llvm::ConstantDataArray::getString(context, string_value);
        // Global pointer that points to the constant array
        // This is owned by the llvm module and doesn't have to be freed manually
        llvm::GlobalVariable* global_variable = new llvm::GlobalVariable(*current_llvm_module, data->getType(), true, llvm::GlobalValue::PrivateLinkage, data);

        // Optimisation: tell llvm that the pointer is never going to be compared
        // (only the value of the string is going to be compared, never the reference to the string)
        global_variable->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        // Create i8* to the first element of the constant array
        llvm::Constant* zero = llvm::ConstantInt::get(get_llvm_type(type_table.Builtins.i32_type_id), 0);
        llvm::Constant* indices[] = {zero, zero};
        llvm::Constant* value = llvm::ConstantExpr::getInBoundsGetElementPtr(data->getType(), global_variable, indices);

        // If strings should be character arrays, the global_variable could be returned (that copies the data)
        return {.llvm_value = value, .returns = false};
    }

    CodegenResult CodegenPass::codegen_binary_expression(const BinaryExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->target_type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        const CodegenResult lhs_cr = codegen_node(expression->lhs.get());
        KPL_ASSERT_NOT_NULLPTR(lhs_cr.llvm_value);
        const CodegenResult rhs_cr = codegen_node(expression->rhs.get());
        KPL_ASSERT_NOT_NULLPTR(rhs_cr.llvm_value);

        const Type* target_type = type_table.lookup(expression->target_type_id);
        switch (expression->operator_type) {
            case OperatorType::Plus:
                return {.llvm_value = create_add(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::Minus:
                return {.llvm_value = create_sub(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::Multiplication:
                return {.llvm_value = create_mul(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::Division:
                return {.llvm_value = create_div(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::LessThan:
                return {.llvm_value = create_less_than(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::GreaterThan:
                return {.llvm_value = create_greater_than(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::Equals:
                return {.llvm_value = create_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::NotEquals:
                return {.llvm_value = create_not_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::LessEquals:
                return {.llvm_value = create_less_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
            case OperatorType::GreaterEquals:
                return {.llvm_value = create_greater_equals(lhs_cr.llvm_value, rhs_cr.llvm_value, target_type, builder), .returns = false};
        }
        KPL_ASSERT_UNREACHABLE("Missing codegen implementation for operator type '{}'", expression->operator_type);
    }

    CodegenResult CodegenPass::codegen_call_expression(const CallExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(expression->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(llvm_values.contains(expression->symbol_id));
        llvm::Function* llvm_function = static_cast<llvm::Function*>(llvm_values[expression->symbol_id]);
        // llvm can't create a call to a function in a different module
        // That's why we use getOrInsertFunction to create an extern declaration if it doesn't exist
        llvm::FunctionCallee llvm_function_callee = current_llvm_module->getOrInsertFunction(llvm_function->getName(), llvm_function->getFunctionType());
        const Symbol* symbol = symbol_table.lookup(expression->symbol_id);
        KPL_ASSERT_THAT(std::holds_alternative<PrototypeSymbolData>(symbol->data));
        bool is_variadic = std::get<PrototypeSymbolData>(symbol->data).is_variadic;
        KPL_ASSERT_THAT(is_variadic == llvm_function->isVarArg(), "Internal variadic: {}, LLVM variadic: {}", is_variadic, llvm_function->isVarArg());
        if (!is_variadic) {
            KPL_ASSERT_THAT(expression->args.size() == llvm_function->arg_size(),
                "Internal arg count: {}, LLVM arg count: {}",
                expression->args.size(),
                llvm_function->arg_size());
        }

        std::vector<llvm::Value*> arg_values;
        for (const std::unique_ptr<Expression>& arg : expression->args) {
            KPL_ASSERT_NOT_NULLPTR(arg);
            const CodegenResult codegen_result = codegen_node(arg.get());
            KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
            arg_values.push_back(codegen_result.llvm_value);
        }

        llvm::Value* value = nullptr;
        KPL_ASSERT_THAT(symbol->type_id != TypeId::invalid());
        KPL_ASSERT_THAT(symbol->type_id != type_table.Builtins.unknown_type_id);
        if (symbol->type_id == type_table.Builtins.void_type_id) {
            value = builder.CreateCall(llvm_function_callee, std::move(arg_values));
        } else {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            KPL_ASSERT_THAT(!identifier.empty());
            value = builder.CreateCall(llvm_function_callee, std::move(arg_values), "call_" + std::string(identifier));
        }
        return {.llvm_value = value, .returns = false};
    }

    CodegenResult CodegenPass::codegen_cast_expression(const CastExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->original_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->original_type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->target_type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        const CodegenResult codegen_result = codegen_node(expression->expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        if (expression->original_type_id == expression->target_type_id) {
            // Redundant cast, so just return the original value
            return {.llvm_value = codegen_result.llvm_value, .returns = false};
        }
        const Type* original_type = type_table.lookup(expression->original_type_id);
        const Type* target_type = type_table.lookup(expression->target_type_id);
        // TODO (fix): reimplement casts
        /*return {
            .llvm_value = create_cast(codegen_result.llvm_value,
                original_type,
                target_type,
                context,
                builder),
            .returns = false,
        };*/
        return {.llvm_value = nullptr, .returns = false};
    }

    CodegenResult CodegenPass::codegen_mathematical_negation_expression(const MathematicalNegationExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        const CodegenResult codegen_result = codegen_node(expression->expression.get());
        KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
        const Type* target_type = type_table.lookup(expression->target_type_id);
        if (is_integer_type(target_type)) {
            return {.llvm_value = builder.CreateNeg(codegen_result.llvm_value), .returns = false};
        } else if (is_floating_point_type(target_type)) {
            return {.llvm_value = builder.CreateFNeg(codegen_result.llvm_value), .returns = false};
        }
        KPL_ASSERT_UNREACHABLE("Missing create mathematical negation implementation for type '{}'", *target_type);
    }

    CodegenResult CodegenPass::codegen_object_initializer_expression(const ObjectInitializerExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(llvm_types.contains(expression->type_id));
        const Type* type = type_table.lookup(expression->type_id);
        KPL_ASSERT_THAT(type->type_kind == TypeKind::Struct);
        const StructType* struct_type = static_cast<const StructType*>(type);

        llvm::Type* llvm_type = llvm_types[expression->type_id];
        KPL_ASSERT_THAT(llvm::isa<llvm::StructType>(llvm_type));
        llvm::StructType* llvm_struct_type = llvm::cast<llvm::StructType>(llvm_type);

        llvm::Value* llvm_value = llvm::Constant::getNullValue(llvm_struct_type);
        for (const MemberInitializerData& member_initializer_data : expression->member_initializers) {
            KPL_ASSERT_NOT_NULLPTR(member_initializer_data.value_expression);
            KPL_ASSERT_THAT(member_initializer_data.identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(member_initializer_data.value_expression->node_type != ASTNodeType::Poison);
            const CodegenResult codegen_result = codegen_node(member_initializer_data.value_expression.get());
            KPL_ASSERT_NOT_NULLPTR(codegen_result.llvm_value);
            const uint32_t field_index = struct_type->get_member_index(member_initializer_data.identifier_id);
            llvm_value = builder.CreateInsertValue(llvm_value, codegen_result.llvm_value, {field_index});
        }
        return {.llvm_value = llvm_value, .returns = false};
    }

    CodegenResult CodegenPass::codegen_variable_expression(const VariableExpression* expression) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(expression->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(llvm_values.contains(expression->symbol_id));
        KPL_ASSERT_THAT(llvm::isa<llvm::AllocaInst>(llvm_values[expression->symbol_id]));

        const Symbol* symbol = symbol_table.lookup(expression->symbol_id);
        KPL_ASSERT_THAT(symbol->type_id != TypeId::invalid());
        KPL_ASSERT_THAT(symbol->type_id != type_table.Builtins.unknown_type_id);
#ifndef NDEBUG
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        KPL_ASSERT_THAT(!identifier.empty());
        llvm::Value* value = builder.CreateLoad(get_llvm_type(symbol->type_id), llvm_values[expression->symbol_id], identifier);
#else
        llvm::Value* value = builder.CreateLoad(get_llvm_type(symbol->type_id), llvm_values[expression->symbol_id]);
#endif
        return {.llvm_value = value, .returns = false};
    }

}
