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

    void NameResolutionPass::create_prototype_symbol(const Prototype* prototype) const {
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

    void NameResolutionPass::resolve_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            resolve_node(node.get());
        }
    }

    void NameResolutionPass::resolve_node(const ASTNode* node) const {
        switch (node->node_type) {
            case ASTNodeType::Extern:
                resolve_extern(static_cast<const Extern*>(node));
                break;
            case ASTNodeType::Function:
                resolve_function(static_cast<const Function*>(node));
                break;
            case ASTNodeType::Prototype:
                resolve_prototype(static_cast<const Prototype*>(node));
                break;
            case ASTNodeType::AssignmentStatement:
                resolve_assignment_statement(static_cast<const AssignmentStatement*>(node));
                break;
            case ASTNodeType::ForStatement:
                resolve_for_statement(static_cast<const ForStatement*>(node));
                break;
            case ASTNodeType::IfStatement:
                resolve_if_statement(static_cast<const IfStatement*>(node));
                break;
            case ASTNodeType::ReturnStatement:
                resolve_return_statement(static_cast<const ReturnStatement*>(node));
                break;
            case ASTNodeType::VariableDefinitionStatement:
                resolve_variable_definition_statement(static_cast<const VariableDefinitionStatement*>(node));
                break;
            case ASTNodeType::BinaryExpression:
                resolve_binary_expression(static_cast<const BinaryExpression*>(node));
                break;
            case ASTNodeType::CallExpression:
                resolve_call_expression(static_cast<const CallExpression*>(node));
                break;
            case ASTNodeType::CastExpression:
                resolve_cast_expression(static_cast<const CastExpression*>(node));
                break;
            case ASTNodeType::MathematicalNegationExpression:
                resolve_mathematical_negation_expression(static_cast<const MathematicalNegationExpression*>(node));
                break;
            case ASTNodeType::VariableExpression:
                resolve_variable_expression(static_cast<const VariableExpression*>(node));
                break;

            // Literals cannot be analysed for symbols
            case ASTNodeType::BooleanLiteralExpression:
            case ASTNodeType::FloatingPointLiteralExpression:
            case ASTNodeType::IntegerLiteralExpression:
            case ASTNodeType::StringLiteralExpression:
                break;

            default:
                KPL_ASSERT(false,
                    "I have no idea how I am supposed to semantically analyse an ast node of type '{}'",
                    node->node_type);
                std::unreachable();
        }
    }

    void NameResolutionPass::resolve_extern(const Extern* ext) const {
        symbol_table.open_scope(ScopeType::Function);
        resolve_prototype(ext->prototype.get());
        symbol_table.close_scope();
    }

    void NameResolutionPass::resolve_function(const Function* function) const {
        symbol_table.open_scope(ScopeType::Function);
        resolve_prototype(function->prototype.get());
        resolve_nodes(function->body);
        symbol_table.close_scope();
    }

    void NameResolutionPass::resolve_prototype(const Prototype* prototype) const {
        for (const ParameterData& parameter : prototype->parameter_data) {
            const auto symbol = symbol_table.create_variable(parameter.data_type, parameter.identifier_id, parameter.source_location);
            if (!symbol) {
                const SourceDiagnostic& diagnostic = symbol.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
            }
        }
    }

    void NameResolutionPass::resolve_assignment_statement(const AssignmentStatement* statement) const {
        resolve_variable_expression(statement->variable_expression.get());
        resolve_node(statement->value_expression.get());
    }

    void NameResolutionPass::resolve_for_statement(const ForStatement* statement) const {
        resolve_variable_definition_statement(statement->loop_variable_definition.get());
        resolve_node(statement->end_value.get());
        resolve_node(statement->step_value.get());
        resolve_nodes(statement->body);
    }

    void NameResolutionPass::resolve_if_statement(const IfStatement* statement) const {
        resolve_node(statement->condition.get());
        resolve_nodes(statement->if_body);
        resolve_nodes(statement->else_body);
    }

    void NameResolutionPass::resolve_return_statement(const ReturnStatement* statement) const {
        resolve_node(statement->expression.get());
    }

    void NameResolutionPass::resolve_variable_definition_statement(const VariableDefinitionStatement* statement) const {
        const auto symbol = symbol_table.create_variable(statement->data_type, statement->identifier_id, statement->source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
        }
        resolve_assignment_statement(statement->assignment_statement.get());
    }

    void NameResolutionPass::resolve_binary_expression(const BinaryExpression* expression) const {
        resolve_node(expression->lhs.get());
        resolve_node(expression->rhs.get());
    }

    void NameResolutionPass::resolve_call_expression(const CallExpression* expression) const {
        if (!symbol_table.lookup(expression->identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
        }
        for (const auto& arg : expression->args) {
            resolve_node(arg.get());
        }
    }

    void NameResolutionPass::resolve_cast_expression(const CastExpression* expression) const {
        resolve_node(expression->expression.get());
    }

    void NameResolutionPass::resolve_mathematical_negation_expression(const MathematicalNegationExpression* expression) const {
        resolve_node(expression->expression.get());
    }

    void NameResolutionPass::resolve_variable_expression(const VariableExpression* expression) const {
        if (!symbol_table.lookup(expression->identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UndefinedSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
        }
    }

}
