// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/name_resolution_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
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
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "io/file.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    NameResolutionPass::NameResolutionPass(DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table)
        : diagnostic_sink(diagnostic_sink), symbol_table(symbol_table), type_table(type_table) {
        if (builtin_type_identifiers.empty()) {
            register_builtin_type_identifier(type_table.Builtins.void_type_id);
            register_builtin_type_identifier(type_table.Builtins.bool_type_id);
            register_builtin_type_identifier(type_table.Builtins.string_type_id);
            register_builtin_type_identifier(type_table.Builtins.i8_type_id);
            register_builtin_type_identifier(type_table.Builtins.i16_type_id);
            register_builtin_type_identifier(type_table.Builtins.i32_type_id);
            register_builtin_type_identifier(type_table.Builtins.i64_type_id);
            register_builtin_type_identifier(type_table.Builtins.u8_type_id);
            register_builtin_type_identifier(type_table.Builtins.u16_type_id);
            register_builtin_type_identifier(type_table.Builtins.u32_type_id);
            register_builtin_type_identifier(type_table.Builtins.u64_type_id);
            register_builtin_type_identifier(type_table.Builtins.f32_type_id);
            register_builtin_type_identifier(type_table.Builtins.f64_type_id);
        }
    }

    void NameResolutionPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            create_struct_symbols_and_types(ast, ast.module_statement->module_id);
            create_prototype_symbols(ast, ast.module_statement->module_id);
        }

        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            module_id = ast.module_statement->module_id;
            resolve_nodes(ast.top_level_nodes);
        }

        // Cleanup state
        module_id = ModuleId::invalid();
    }

    void NameResolutionPass::register_builtin_type_identifier(TypeId type_id) {
        const Type* type = type_table.lookup(type_id);
        builtin_type_identifiers.emplace(type->identifier_id, type_id);
    }

    void NameResolutionPass::create_struct_symbols_and_types(const AbstractSyntaxTree& ast, ModuleId module_id) {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (const std::unique_ptr<Struct>& struct_node : ast.struct_nodes) {
            const auto symbol_id = symbol_table.create_struct(module_id, struct_node->identifier_id);
            if (!symbol_id) {
                const Diagnostic& diagnostic = symbol_id.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, struct_node->source_location);
                struct_node->node_type = ASTNodeType::Poison;
                continue;
            }
            const TypeId type_id = type_table.create_struct(struct_node->identifier_id, struct_node->members);
            Symbol* symbol = symbol_table.lookup(*symbol_id);
            symbol->type_id = type_id;
        }
    }

    void NameResolutionPass::create_prototype_symbols(const AbstractSyntaxTree& ast, ModuleId module_id) {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(ext->prototype);
                    create_prototype_symbol(module_id, ext->prototype.get(), ext->linkage_type);
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(function->prototype);
                    create_prototype_symbol(module_id, function->prototype.get(), function->linkage_type);
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type of type '{}' on top level during name resolution", node->node_type);
            }
        }
    }

    void NameResolutionPass::create_prototype_symbol(ModuleId module_id, Prototype* prototype, LinkageType linkage_type) const {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(prototype->return_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);
        const std::optional<TypeId> return_type_id = resolve_identifier_to_type_id(prototype->return_type_identifier_id, prototype->source_location);
        if (!return_type_id.has_value()) {
            prototype->node_type = ASTNodeType::Poison;
            return;
        }
        KPL_ASSERT_THAT(return_type_id.value() != TypeId::invalid());
        prototype->return_type_id = return_type_id.value();

        std::vector<TypeId> parameter_type_ids;
        parameter_type_ids.reserve(prototype->parameter_data.size());
        for (ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type_id == TypeId::invalid());
            KPL_ASSERT_THAT(parameter_data.type_identifier_id != StringId::invalid());
            const std::optional<TypeId> parameter_type_id = resolve_identifier_to_type_id(parameter_data.type_identifier_id,
                parameter_data.type_source_location);
            if (!parameter_type_id.has_value()) {
                prototype->node_type = ASTNodeType::Poison;
                return;
            }
            KPL_ASSERT_THAT(parameter_type_id != TypeId::invalid());
            parameter_data.type_id = parameter_type_id.value();
            parameter_type_ids.push_back(parameter_type_id.value());
        }

        const auto symbol_id = symbol_table.create_prototype(module_id,
            return_type_id.value(),
            prototype->identifier_id,
            linkage_type,
            std::move(parameter_type_ids),
            prototype->is_variadic);
        if (!symbol_id) {
            const Diagnostic& diagnostic = symbol_id.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, prototype->identifier_source_location);
            prototype->node_type = ASTNodeType::Poison;
        } else {
            prototype->symbol_id = *symbol_id;
        }
    }

    NameResolutionResult NameResolutionPass::resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        bool poisoned = false;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            const NameResolutionResult resolution_result = resolve_node(node.get());
            if (resolution_result.poisoned) {
                poisoned = true;
            }
        }
        return {.poisoned = poisoned};
    }

    NameResolutionResult NameResolutionPass::resolve_node(ASTNode* node) const {
        KPL_ASSERT_NOT_NULLPTR(node);
        switch (node->node_type) {
            case ASTNodeType::Struct:
            case ASTNodeType::Prototype:
            case ASTNodeType::ImportStatement:
            case ASTNodeType::ModuleStatement:
                KPL_ASSERT_UNREACHABLE("Cannot do name resolution for a node of type '{}' as part of the top level nodes", node->node_type);

            // Literals cannot be analysed for symbols
            case ASTNodeType::BooleanLiteralExpression:
            case ASTNodeType::FloatingPointLiteralExpression:
            case ASTNodeType::IntegerLiteralExpression:
            case ASTNodeType::StringLiteralExpression:
                return {.poisoned = false};

            case ASTNodeType::Poison:
                return {.poisoned = true};
            case ASTNodeType::Extern:
                resolve_extern(static_cast<Extern*>(node));
                return {.poisoned = false};
            case ASTNodeType::Function:
                resolve_function(static_cast<Function*>(node));
                return {.poisoned = false};
            case ASTNodeType::AssignmentStatement:
                return resolve_assignment_statement(static_cast<AssignmentStatement*>(node));
            case ASTNodeType::ForStatement:
                return resolve_for_statement(static_cast<ForStatement*>(node));
            case ASTNodeType::IfStatement:
                return resolve_if_statement(static_cast<IfStatement*>(node));
            case ASTNodeType::ReturnStatement:
                return resolve_return_statement(static_cast<ReturnStatement*>(node));
            case ASTNodeType::VariableDefinitionStatement:
                return resolve_variable_definition_statement(static_cast<VariableDefinitionStatement*>(node));
            case ASTNodeType::BinaryExpression:
                return resolve_binary_expression(static_cast<BinaryExpression*>(node));
            case ASTNodeType::CallExpression:
                return resolve_call_expression(static_cast<CallExpression*>(node));
            case ASTNodeType::CastExpression:
                return resolve_cast_expression(static_cast<CastExpression*>(node));
            case ASTNodeType::MathematicalNegationExpression:
                return resolve_mathematical_negation_expression(static_cast<MathematicalNegationExpression*>(node));
            case ASTNodeType::VariableExpression:
                return resolve_variable_expression(static_cast<VariableExpression*>(node));
        }

        KPL_ASSERT_UNREACHABLE("Missing name resolution implementation for node of type '{}'", node->node_type);
    }

    void NameResolutionPass::resolve_extern(Extern* ext) const {
        KPL_ASSERT_NOT_NULLPTR(ext);
        KPL_ASSERT_NOT_NULLPTR(ext->prototype);
        KPL_ASSERT_THAT(ext->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Function);
        const NameResolutionResult prototype_result = resolve_prototype(ext->prototype.get());
        symbol_table.close_scope(module_id);
        if (prototype_result.poisoned) {
            ext->node_type = ASTNodeType::Poison;
        }
    }

    void NameResolutionPass::resolve_function(Function* function) const {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Function);
        resolve_prototype(function->prototype.get());
        resolve_nodes(function->body.nodes);
        symbol_table.close_scope(module_id);
    }

    NameResolutionResult NameResolutionPass::resolve_prototype(Prototype* prototype) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        // Normally this assert would be there,
        // but because function overloading isn't implemented yet, prototypes poison themselves when trying to overload a function
        // KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(parameter_data.identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(parameter_data.symbol_id == SymbolId::invalid());
            const auto symbol = symbol_table.create_variable(module_id, parameter_data.type_id, parameter_data.identifier_id);
            if (!symbol) {
                const Diagnostic& diagnostic = symbol.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, parameter_data.identifier_source_location);
                prototype->node_type = ASTNodeType::Poison;
            } else {
                parameter_data.symbol_id = *symbol;
            }
        }
        return {.poisoned = prototype->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_assignment_statement(AssignmentStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        const NameResolutionResult variable_nrr = resolve_variable_expression(statement->variable_expression.get());
        const NameResolutionResult value_nrr = resolve_node(statement->value_expression.get());
        if (variable_nrr.poisoned || value_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_for_statement(ForStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult definition_nrr = resolve_variable_definition_statement(statement->loop_variable_definition.get());
        const NameResolutionResult end_nrr = resolve_node(statement->end_value.get());
        NameResolutionResult step_nrr{.poisoned = false};
        if (statement->step_value != nullptr) {
            step_nrr = resolve_node(statement->step_value.get());
        }
        const NameResolutionResult body_nrr = resolve_nodes(statement->body.nodes);
        symbol_table.close_scope(module_id);

        if (definition_nrr.poisoned || end_nrr.poisoned || step_nrr.poisoned || body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_if_statement(IfStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const NameResolutionResult condition_nrr = resolve_node(statement->condition.get());

        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult if_body_nrr = resolve_nodes(statement->if_body.nodes);
        symbol_table.close_scope(module_id);

        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult else_body_nrr = resolve_nodes(statement->else_body.nodes);
        symbol_table.close_scope(module_id);

        if (condition_nrr.poisoned || if_body_nrr.poisoned || else_body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_return_statement(ReturnStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        if (statement->expression == nullptr) {
            return {.poisoned = false};
        }

        const NameResolutionResult resolution_result = resolve_node(statement->expression.get());
        if (resolution_result.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_variable_definition_statement(VariableDefinitionStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_THAT(statement->type_id != TypeId::invalid());
        KPL_ASSERT_THAT(statement->type_identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(statement->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const std::optional<TypeId> type_id = resolve_identifier_to_type_id(statement->type_identifier_id, statement->source_location);
        if (!type_id.has_value()) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        statement->type_id = type_id.value();

        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement->variable_expression);
        const auto symbol = symbol_table.create_variable(module_id, type_id.value(), statement->identifier_id);
        if (!symbol) {
            const Diagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, statement->assignment_statement->variable_expression->source_location);
            statement->node_type = ASTNodeType::Poison;
        }

        const NameResolutionResult resolution_result = resolve_assignment_statement(statement->assignment_statement.get());
        if (resolution_result.poisoned) {
            statement->node_type = ASTNodeType::Poison;
        }
        // Return like this because the node can already poison itself if the symbol creation failed
        return {.poisoned = statement->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_binary_expression(BinaryExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const NameResolutionResult lhs_nrr = resolve_node(expression->lhs.get());
        const NameResolutionResult rhs_nrr = resolve_node(expression->rhs.get());
        if (lhs_nrr.poisoned || rhs_nrr.poisoned) {
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_call_expression(CallExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(expression->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        std::expected<Symbol*, Diagnostic> prototype_symbol;
        if (expression->module_path.part_identifier_ids.empty()) {
            prototype_symbol = symbol_table.find_symbol(module_id, expression->identifier_id);
            if (prototype_symbol.has_value() && prototype_symbol.value() == nullptr) {
                const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
                diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
        } else {
            prototype_symbol = symbol_table.find_symbol(module_id, expression->module_path, expression->identifier_id);
            if (prototype_symbol.has_value() && prototype_symbol.value() == nullptr) {
                KPL_ASSERT_THAT(expression->module_source_location.file_id != FileId::invalid());
                KPL_ASSERT_THAT(expression->module_source_location.size > 0);
                const std::string message = std::format("Unknown module '{}'", get_full_module_identifier(expression->module_path));
                diagnostic_sink.report(DiagnosticCode::UnknownModule, std::move(message), expression->module_source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
        }

        if (!prototype_symbol) {
            const Diagnostic diagnostic = prototype_symbol.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        KPL_ASSERT_NOT_NULLPTR(prototype_symbol.value());
        expression->symbol_id = prototype_symbol.value()->id;
        KPL_ASSERT_THAT(std::holds_alternative<PrototypeSymbolData>(prototype_symbol.value()->data));
        const PrototypeSymbolData& prototype_symbol_data = std::get<PrototypeSymbolData>(prototype_symbol.value()->data);
        if (!prototype_symbol_data.is_variadic) {
            const size_t expected_parameter_count = prototype_symbol_data.parameter_type_ids.size();
            const size_t given_argument_count = expression->args.size();
            if (expected_parameter_count != given_argument_count) {
                const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
                const std::string message = std::format("Function '{}' expects {} arguments, got {}",
                    identifier,
                    expected_parameter_count,
                    given_argument_count);
                diagnostic_sink.report(DiagnosticCode::InvalidFunctionCall, message, expression->source_location);
                return {.poisoned = true};
            }
        }

        bool poisoned = false;
        for (const std::unique_ptr<Expression>& arg : expression->args) {
            KPL_ASSERT_NOT_NULLPTR(arg);
            const NameResolutionResult resolution_result = resolve_node(arg.get());
            if (resolution_result.poisoned) {
                poisoned = true;
            }
        }
        if (poisoned) {
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_cast_expression(CastExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->target_type_identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const std::optional<TypeId> target_type_id = resolve_identifier_to_type_id(expression->target_type_identifier_id, expression->source_location);
        if (!target_type_id.has_value()) {
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        KPL_ASSERT_THAT(target_type_id.value() != TypeId::invalid());
        expression->target_type_id = target_type_id.value();

        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result.poisoned) {
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result.poisoned) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_variable_expression(VariableExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(expression->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const auto symbol = symbol_table.find_symbol(module_id, expression->identifier_id);
        if (!symbol) {
            const Diagnostic diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        if (symbol == nullptr) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            expression->symbol_id = symbol.value()->id;
        }

        return {.poisoned = false};
    }

    std::optional<TypeId> NameResolutionPass::resolve_identifier_to_type_id(StringId type_identifier_id, SourceLocation source_location) const {
        KPL_ASSERT_THAT(type_identifier_id != StringId::invalid());
        const auto it = builtin_type_identifiers.find(type_identifier_id);
        if (it != builtin_type_identifiers.end()) {
            return it->second;
        }

        KPL_ASSERT_THAT(type_identifier_id != type_table.lookup(type_table.Builtins.unknown_type_id)->identifier_id);
        const auto type_symbol = symbol_table.find_symbol(module_id, type_identifier_id);
        if (!type_symbol.has_value()) {
            const Diagnostic& diagnostic = type_symbol.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), std::move(source_location));
            return std::nullopt;
        }
        if (type_symbol.value() == nullptr) {
            const std::string_view type_name = StringPool::get().lookup(type_identifier_id);
            diagnostic_sink.report(DiagnosticCode::UnknownType, std::format("Unknown type '{}'", type_name), std::move(source_location));
            return std::nullopt;
        }
        return type_symbol.value()->type_id;
    }

}
