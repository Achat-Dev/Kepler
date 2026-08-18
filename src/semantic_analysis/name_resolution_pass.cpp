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
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    void NameResolutionPass::run() {
        collect_prototype_symbols();
        resolve_nodes(ast.top_level_nodes);
    }

    void NameResolutionPass::collect_prototype_symbols() const {
        for (std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(node.get());
                    create_prototype_symbol(ext->prototype.get());
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(node.get());
                    create_prototype_symbol(function->prototype.get());
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type of type '{}' on top level during name resolution", node->node_type);
            }
        }
    }

    NameResolutionResult NameResolutionPass::create_prototype_symbol(Prototype* prototype) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison, "Can't create prototype symbol from poisoned prototype");
        Type* return_type = type_table.lookup(prototype->return_type_id);
        if (return_type == nullptr) {
            report_unknown_type(prototype->return_type_id, prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(prototype->return_type == nullptr, "Return type of prototype must be nullptr when creating prototype symbol");
            prototype->return_type = return_type;
        }

        std::vector<Type*> parameter_types;
        parameter_types.reserve(prototype->parameter_data.size());
        for (auto& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type == nullptr, "Prototype parameter type must be nullptr when creating prototype symbol");
            Type* parameter_type = type_table.lookup(parameter_data.type_id);
            if (parameter_type == nullptr) {
                report_unknown_type(parameter_data.type_id, parameter_data.type_source_location);
                prototype->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
            parameter_data.type = parameter_type;
            parameter_types.push_back(parameter_type);
        }

        const auto symbol = symbol_table.create_prototype(return_type,
            prototype->identifier_id,
            prototype->linkage_type,
            std::move(parameter_types),
            prototype->identifier_source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(prototype->symbol == nullptr, "Prototype symbol must be nullptr when creating prototype symbol");
            prototype->symbol = *symbol;
        }

        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        bool poisoned = false;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
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
            case ASTNodeType::Poison:
                return {.poisoned = true};
            case ASTNodeType::Extern:
                resolve_extern(static_cast<Extern*>(node));
                return {.poisoned = false};
            case ASTNodeType::Function:
                resolve_function(static_cast<Function*>(node));
                return {.poisoned = false};
            case ASTNodeType::Prototype:
                return resolve_prototype(static_cast<Prototype*>(node));
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

            // Literals cannot be analysed for symbols
            case ASTNodeType::BooleanLiteralExpression:
            case ASTNodeType::FloatingPointLiteralExpression:
            case ASTNodeType::IntegerLiteralExpression:
            case ASTNodeType::StringLiteralExpression:
                return {.poisoned = false};
        }

        KPL_ASSERT_UNREACHABLE("Missing name resolution implementation for node of type '{}'", node->node_type);
    }

    void NameResolutionPass::resolve_extern(Extern* ext) const {
        KPL_ASSERT_NOT_NULLPTR(ext);
        KPL_ASSERT_NOT_NULLPTR(ext->prototype);
        KPL_ASSERT_THAT(ext->node_type != ASTNodeType::Poison, "Extern must not be poisoned for name resolution");
        symbol_table.open_scope(ScopeType::Function);
        const NameResolutionResult prototype_result = resolve_prototype(ext->prototype.get());
        symbol_table.close_scope();
        if (prototype_result.poisoned) {
            ext->node_type = ASTNodeType::Poison;
        }
    }

    void NameResolutionPass::resolve_function(Function* function) const {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison, "Function must not be poisoned for name resolution");
        symbol_table.open_scope(ScopeType::Function);
        resolve_prototype(function->prototype.get());
        resolve_nodes(function->body);
        symbol_table.close_scope();
    }

    NameResolutionResult NameResolutionPass::resolve_prototype(Prototype* prototype) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        // Normally this assert would be there,
        // but because function overloading isn't implemented yet, prototypes poison themselves when trying to overload a function
        // KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison, "Prototype must not be poisoned for name resolution");

        for (ParameterData& parameter : prototype->parameter_data) {
            KPL_ASSERT_NOT_NULLPTR(parameter.type);
            const auto symbol = symbol_table.create_variable(parameter.type, parameter.identifier_id, parameter.identifier_source_location);
            if (!symbol) {
                const SourceDiagnostic& diagnostic = symbol.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
                prototype->node_type = ASTNodeType::Poison;
            } else {
                KPL_ASSERT_THAT(parameter.symbol == nullptr, "Symbol of prototype parameter must be nullptr for name resolution");
                parameter.symbol = *symbol;
            }
        }
        return {.poisoned = prototype->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_assignment_statement(AssignmentStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "AssignmentStatement must not be posioned for name resolution");
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
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ForStatement must not be poisoned for name resolution");
        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult definition_nrr = resolve_variable_definition_statement(statement->loop_variable_definition.get());
        const NameResolutionResult end_nrr = resolve_node(statement->end_value.get());
        NameResolutionResult step_nrr{.poisoned = false};
        if (statement->step_value != nullptr) {
            step_nrr = resolve_node(statement->step_value.get());
        }
        const NameResolutionResult body_nrr = resolve_nodes(statement->body);
        symbol_table.close_scope();

        if (definition_nrr.poisoned || end_nrr.poisoned || step_nrr.poisoned || body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_if_statement(IfStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "IfStatement must not be poisoned for name resolution");
        const NameResolutionResult condition_nrr = resolve_node(statement->condition.get());

        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult if_body_nrr = resolve_nodes(statement->if_body);
        symbol_table.close_scope();

        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult else_body_nrr = resolve_nodes(statement->else_body);
        symbol_table.close_scope();

        if (condition_nrr.poisoned || if_body_nrr.poisoned || else_body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_return_statement(ReturnStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ReturnStatement must not be poisoned for name resolution");
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
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "VariableDefinitionStatement must not be poisoned for name resolution");
        Type* type = type_table.lookup(statement->type_id);
        if (type == nullptr) {
            report_unknown_type(statement->type_id, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(statement->type == nullptr, "Type of VariableDefinitionStatement must be nullptr for name resolution");
            statement->type = type;
        }

        const auto symbol = symbol_table.create_variable(type, statement->identifier_id, statement->source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
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
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BinaryExpression must not be poisoned for name resolution");
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
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CallExpression must not be poisoned for name resolution");
        Symbol* prototype_symbol = symbol_table.lookup(expression->identifier_id);
        if (prototype_symbol == nullptr) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(expression->symbol == nullptr, "CallExpression symbol must be nullptr for name resolution");
            expression->symbol = prototype_symbol;
        }

        KPL_ASSERT_HOLDS_ALTERNATIVE(prototype_symbol->data, PrototypeSymbolData, "PrototypeSymbolData");
        const PrototypeSymbolData& prototype_symbol_data = std::get<PrototypeSymbolData>(prototype_symbol->data);
        const size_t expected_parameter_count = prototype_symbol_data.parameter_types.size();
        const size_t given_argument_count = expression->args.size();
        if (expected_parameter_count != given_argument_count) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            const std::string message = std::format("Function '{}' expects {} arguments, got {}", identifier, expected_parameter_count, given_argument_count);
            diagnostic_sink.report(DiagnosticCode::InvalidFunctionCall, message, expression->source_location);
            return {.poisoned = true};
        }

        bool poisoned = false;
        for (const auto& arg : expression->args) {
            KPL_ASSERT_NOT_NULLPTR(arg.get());
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
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CastExpression must not be poisoned for name resolution");
        Type* target_type = type_table.lookup(expression->target_type_id);
        if (!target_type) {
            report_unknown_type(expression->target_type_id, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(expression->target_type == nullptr, "Target type of CastExpression must be nullptr for name resolution");
            expression->target_type = target_type;
        }

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
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "MathematicalNegationExpression must not be poisoned for name resolution");
        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result.poisoned) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_variable_expression(VariableExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "VariableExpression must not be poisoned for name resolution");
        Symbol* symbol = symbol_table.lookup(expression->identifier_id);
        if (symbol == nullptr) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_THAT(expression->symbol == nullptr, "Symbol of VariableExpression must be nullptr for name resolution");
            expression->symbol = symbol;
        }

        return {.poisoned = false};
    }

    void NameResolutionPass::report_unknown_type(StringId type_id, SourceLocation source_location) const {
        const std::string_view type_name = StringPool::get().lookup(type_id);
        diagnostic_sink.report(DiagnosticCode::UnknownType, std::format("Unknown type '{}'", type_name), source_location);
    }

}
