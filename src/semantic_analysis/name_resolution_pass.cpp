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
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/string_pool.hpp"
#include <cassert>
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
                    assert(false && "Invalid ast node type on top level during name resolution");
                    std::unreachable();
            }
        }
    }

    NameResolutionResult NameResolutionPass::create_prototype_symbol(Prototype* prototype) const {
        assert(prototype->return_type == nullptr);
        Type* return_type = type_table.lookup(prototype->return_type_id);
        if (return_type == nullptr) {
            report_unknown_type(prototype->return_type_id, prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {true};
        }
        prototype->return_type = return_type;

        std::vector<Type*> parameter_types;
        parameter_types.reserve(prototype->parameter_data.size());
        for (auto& parameter_data : prototype->parameter_data) {
            assert(parameter_data.type == nullptr);
            Type* parameter_type = type_table.lookup(parameter_data.type_id);
            if (parameter_type == nullptr) {
                report_unknown_type(parameter_data.type_id, parameter_data.type_source_location);
                prototype->node_type = ASTNodeType::Poison;
                return {true};
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
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        bool poisoned = false;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            const NameResolutionResult resolution_result = resolve_node(node.get());
            if (resolution_result) {
                poisoned = true;
            }
        }
        return {poisoned};
    }

    NameResolutionResult NameResolutionPass::resolve_node(ASTNode* node) const {
        if (node == nullptr) {
            return {false};
        }

        switch (node->node_type) {
            case ASTNodeType::Extern:
                resolve_extern(static_cast<Extern*>(node));
                return {false};
            case ASTNodeType::Function:
                resolve_function(static_cast<Function*>(node));
                return {false};
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
                return {false};

            default:
                assert(false && "Invalid node type for name resolution");
                std::unreachable();
        }
    }

    void NameResolutionPass::resolve_extern(Extern* ext) const {
        symbol_table.open_scope(ScopeType::Function);
        const NameResolutionResult prototype_result = resolve_prototype(ext->prototype.get());
        symbol_table.close_scope();

        if (prototype_result) {
            ext->node_type = ASTNodeType::Poison;
        }
    }

    void NameResolutionPass::resolve_function(Function* function) const {
        symbol_table.open_scope(ScopeType::Function);
        resolve_prototype(function->prototype.get());
        resolve_nodes(function->body);
        symbol_table.close_scope();
    }

    NameResolutionResult NameResolutionPass::resolve_prototype(Prototype* prototype) const {
        for (const ParameterData& parameter : prototype->parameter_data) {
            const auto symbol = symbol_table.create_variable(parameter.type, parameter.identifier_id, parameter.identifier_source_location);
            if (!symbol) {
                const SourceDiagnostic& diagnostic = symbol.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
                prototype->node_type = ASTNodeType::Poison;
            }
        }
        return {prototype->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_assignment_statement(AssignmentStatement* statement) const {
        const NameResolutionResult variable_nrr = resolve_variable_expression(statement->variable_expression.get());
        const NameResolutionResult value_nrr = resolve_node(statement->value_expression.get());
        if (variable_nrr || value_nrr) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_for_statement(ForStatement* statement) const {
        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult definition_nrr = resolve_variable_definition_statement(statement->loop_variable_definition.get());
        const NameResolutionResult end_nrr = resolve_node(statement->end_value.get());
        const NameResolutionResult step_nrr = resolve_node(statement->step_value.get());
        const NameResolutionResult body_nrr = resolve_nodes(statement->body);
        symbol_table.close_scope();

        if (definition_nrr || end_nrr || step_nrr || body_nrr) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_if_statement(IfStatement* statement) const {
        const NameResolutionResult condition_nrr = resolve_node(statement->condition.get());

        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult if_body_nrr = resolve_nodes(statement->if_body);
        symbol_table.close_scope();

        symbol_table.open_scope(ScopeType::Block);
        const NameResolutionResult else_body_nrr = resolve_nodes(statement->else_body);
        symbol_table.close_scope();

        if (condition_nrr || if_body_nrr || else_body_nrr) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_return_statement(ReturnStatement* statement) const {
        const NameResolutionResult resolution_result = resolve_node(statement->expression.get());
        if (resolution_result) {
            statement->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_variable_definition_statement(VariableDefinitionStatement* statement) const {
        assert(statement->type == nullptr);
        Type* type = type_table.lookup(statement->type_id);
        if (type == nullptr) {
            report_unknown_type(statement->type_id, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        statement->type = type;

        const auto symbol = symbol_table.create_variable(type, statement->identifier_id, statement->source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
            statement->node_type = ASTNodeType::Poison;
        }

        const NameResolutionResult resolution_result = resolve_assignment_statement(statement->assignment_statement.get());
        if (resolution_result) {
            statement->node_type = ASTNodeType::Poison;
        }
        // Return like this because the node can already poison itself if the symbol creation failed
        return {statement->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_binary_expression(BinaryExpression* expression) const {
        const NameResolutionResult lhs_nrr = resolve_node(expression->lhs.get());
        const NameResolutionResult rhs_nrr = resolve_node(expression->rhs.get());
        if (lhs_nrr || rhs_nrr) {
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_call_expression(CallExpression* expression) const {
        const Symbol* prototype_symbol = symbol_table.lookup_visible(expression->identifier_id);
        if (prototype_symbol == nullptr) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }

        const PrototypeSymbolData& prototype_symbol_data = std::get<PrototypeSymbolData>(prototype_symbol->data);
        const size_t expected_parameter_count = prototype_symbol_data.parameter_types.size();
        const size_t given_argument_count = expression->args.size();
        if (expected_parameter_count != given_argument_count) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            const std::string message = std::format("Function '{}' expects {} arguments, got {}", identifier, expected_parameter_count, given_argument_count);
            diagnostic_sink.report(DiagnosticCode::InvalidFunctionCall, message, expression->source_location);
            return {true};
        }

        bool poisoned = false;
        for (const auto& arg : expression->args) {
            const NameResolutionResult resolution_result = resolve_node(arg.get());
            if (resolution_result) {
                poisoned = true;
            }
        }
        if (poisoned) {
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    NameResolutionResult NameResolutionPass::resolve_cast_expression(CastExpression* expression) const {
        assert(expression->target_type == nullptr);
        Type* target_type = type_table.lookup(expression->target_type_id);
        if (!target_type) {
            report_unknown_type(expression->target_type_id, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        expression->target_type = target_type;

        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const {
        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_variable_expression(VariableExpression* expression) const {
        if (!symbol_table.lookup_visible(expression->identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    void NameResolutionPass::report_unknown_type(StringId type_id, SourceLocation source_location) const {
        const std::string_view type_name = StringPool::get().lookup(type_id);
        diagnostic_sink.report(DiagnosticCode::UnknownType, std::format("Unknown type '{}'", type_name), source_location);
    }

}
