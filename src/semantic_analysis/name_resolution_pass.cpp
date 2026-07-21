// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/name_resolution_pass.hpp"
#include "assert.hpp"
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
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    using Nrr = NameResolutionPass::ResolutionResult;

    void NameResolutionPass::run() {
        collect_prototype_symbols();
        resolve_nodes(ast.nodes);
    }

    void NameResolutionPass::collect_prototype_symbols() const {
        for (const std::unique_ptr<ASTNode>& ast_node : ast.nodes) {
            switch (ast_node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(ast_node.get());
                    create_prototype_symbol(ext->prototype.get());
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(ast_node.get());
                    create_prototype_symbol(function->prototype.get());
                    break;
                }
                default:
                    KPL_ASSERT(false,
                        "Behold: I somehow managed to create a malformed AST with a node of type '{}' on the top level <(˘ ˘ ˘)>",
                        ast_node->node_type);
                    std::unreachable();
            }
        }
    }

    void NameResolutionPass::create_prototype_symbol(Prototype* prototype) const {
        std::vector<DataTypeKind> parameter_data_types;
        parameter_data_types.reserve(prototype->parameter_data.size());
        for (const auto& parameter_data : prototype->parameter_data) {
            parameter_data_types.push_back(parameter_data.data_type);
        }
        const auto symbol = symbol_table.create_prototype(prototype->return_type,
            prototype->identifier_id,
            prototype->linkage_type,
            std::move(parameter_data_types),
            prototype->source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
        }
    }

    Nrr NameResolutionPass::resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        bool poisoned = false;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            const Nrr resolution_result = resolve_node(node.get());
            if (resolution_result) {
                poisoned = true;
            }
        }
        return {poisoned};
    }

    Nrr NameResolutionPass::resolve_node(ASTNode* node) const {
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
                KPL_ASSERT(false,
                    "I have no idea how I am supposed to semantically analyse an ast node of type '{}'",
                    node->node_type);
                std::unreachable();
        }
    }

    void NameResolutionPass::resolve_extern(Extern* ext) const {
        symbol_table.open_scope(ScopeType::Function);
        const Nrr prototype_result = resolve_prototype(ext->prototype.get());
        symbol_table.close_scope();

        if (prototype_result) {
            ext->node_type = ASTNodeType::Poison;
        }
    }

    void NameResolutionPass::resolve_function(Function* function) const {
        symbol_table.open_scope(ScopeType::Function);
        const Nrr prototype_result = resolve_prototype(function->prototype.get());
        const Nrr body_result = resolve_nodes(function->body);
        symbol_table.close_scope();
    }

    Nrr NameResolutionPass::resolve_prototype(Prototype* prototype) const {
        for (const ParameterData& parameter : prototype->parameter_data) {
            const auto symbol = symbol_table.create_variable(parameter.data_type, parameter.identifier_id, parameter.source_location);
            if (!symbol) {
                const SourceDiagnostic& diagnostic = symbol.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
                prototype->node_type = ASTNodeType::Poison;
            }
        }
        return {prototype->node_type == ASTNodeType::Poison};
    }

    Nrr NameResolutionPass::resolve_assignment_statement(AssignmentStatement* statement) const {
        const Nrr variable_result = resolve_variable_expression(statement->variable_expression.get());
        const Nrr value_result = resolve_node(statement->value_expression.get());
        if (variable_result || value_result) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    Nrr NameResolutionPass::resolve_for_statement(ForStatement* statement) const {
        symbol_table.open_scope(ScopeType::Block);
        const Nrr definition_result = resolve_variable_definition_statement(statement->loop_variable_definition.get());
        const Nrr end_result = resolve_node(statement->end_value.get());
        const Nrr step_result = resolve_node(statement->step_value.get());
        const Nrr body_result = resolve_nodes(statement->body);
        symbol_table.close_scope();

        if (definition_result || end_result || step_result || body_result) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    Nrr NameResolutionPass::resolve_if_statement(IfStatement* statement) const {
        const Nrr condition_result = resolve_node(statement->condition.get());

        symbol_table.open_scope(ScopeType::Block);
        const Nrr if_body_result = resolve_nodes(statement->if_body);
        symbol_table.close_scope();

        symbol_table.open_scope(ScopeType::Block);
        const Nrr else_body_result = resolve_nodes(statement->else_body);
        symbol_table.close_scope();

        if (condition_result || if_body_result || else_body_result) {
            statement->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    Nrr NameResolutionPass::resolve_return_statement(ReturnStatement* statement) const {
        const Nrr resolution_result = resolve_node(statement->expression.get());
        if (resolution_result) {
            statement->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    Nrr NameResolutionPass::resolve_variable_definition_statement(VariableDefinitionStatement* statement) const {
        const auto symbol = symbol_table.create_variable(statement->data_type, statement->identifier_id, statement->source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
            statement->node_type = ASTNodeType::Poison;
        }

        const Nrr resolution_result = resolve_assignment_statement(statement->assignment_statement.get());
        if (resolution_result) {
            statement->node_type = ASTNodeType::Poison;
        }
        // Return like this because the node can already poison itself if the symbol creation failed
        return {statement->node_type == ASTNodeType::Poison};
    }

    Nrr NameResolutionPass::resolve_binary_expression(BinaryExpression* expression) const {
        const Nrr lhs_result = resolve_node(expression->lhs.get());
        const Nrr rhs_result = resolve_node(expression->rhs.get());
        if (lhs_result || rhs_result) {
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

    Nrr NameResolutionPass::resolve_call_expression(CallExpression* expression) const {
        if (!symbol_table.lookup(expression->identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
        }

        bool poisoned = false;
        for (const auto& arg : expression->args) {
            const Nrr resolution_result = resolve_node(arg.get());
            if (resolution_result) {
                poisoned = true;
            }
        }
        if (poisoned) {
            expression->node_type = ASTNodeType::Poison;
        }
        // Return like this because the node can already poison itself if the call symbol doesn't exist
        return {expression->node_type == ASTNodeType::Poison};
    }

    Nrr NameResolutionPass::resolve_cast_expression(CastExpression* expression) const {
        const Nrr resolution_result = resolve_node(expression->expression.get());
        if (resolution_result) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    Nrr NameResolutionPass::resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const {
        const Nrr resolution_result = resolve_node(expression->expression.get());
        if (resolution_result) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    Nrr NameResolutionPass::resolve_variable_expression(VariableExpression* expression) const {
        if (!symbol_table.lookup(expression->identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {true};
        }
        return {false};
    }

}
