// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/ast_printer.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/extern.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstring>
#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace kepler {

    // TODO: Update ast nodes to display types and symbols

    void ASTPrinter::run() {
        constexpr char title[] = " Abstract Syntax Tree ";
        std::string horizontal_line;
        for (size_t i = 0; i < strlen(title); i++) {
            horizontal_line += "\u2500";
        }
        std::println("\u250C{}\u2510", horizontal_line);
        std::println("\u2502{}\u2502", title);
        std::println("\u2514{}\u2518", horizontal_line);

        // Dont't use print_nodes to avoid extra 'last_item' character
        for (size_t i = 0; i < ast.top_level_nodes.size(); i++) {
            bool is_last = i == ast.top_level_nodes.size() - 1;
            print_node(ast.top_level_nodes[i].get(), "", "", is_last);
        }
    }

    void ASTPrinter::print_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes, const std::string& label, std::string indent, bool is_last) const {
        if (is_last) {
            std::print("{}{}{}: ", indent, last_item_prefix, label);
            indent += space;
        } else {
            std::print("{}{}{}: ", indent, item_prefix, label);
            indent += vertical_line;
        }

        if (nodes.empty()) {
            std::println("{}Empty{}", ansi_codes::dim, ansi_codes::reset);
            return;
        } else {
            std::println();
        }

        for (size_t i = 0; i < nodes.size(); i++) {
            bool is_last = i == nodes.size() - 1;
            print_node(nodes[i].get(), "", indent, is_last);
        }
    }

    void ASTPrinter::print_node(const ASTNode* node, const std::string& prefix, std::string indent, bool is_last) const {
        KPL_ASSERT_NOT_NULLPTR(node);
        if (node->node_type == ASTNodeType::Poison) {
            std::print(ansi_codes::magenta);
        }

        std::print("{}", indent);
        if (is_last) {
            std::print(last_item_prefix);
            indent += space;
        } else {
            std::print(item_prefix);
            indent += vertical_line;
        }
        std::println("{}{}{}", prefix, node->node_type, ansi_codes::reset);

        switch (node->node_type) {
            case ASTNodeType::Poison:
                return;
            case ASTNodeType::Extern:
                print_extern(static_cast<const Extern*>(node), indent);
                return;
            case ASTNodeType::Function:
                print_function(static_cast<const Function*>(node), indent);
                return;
            case ASTNodeType::Prototype:
                print_prototype(static_cast<const Prototype*>(node), indent);
                return;
            case ASTNodeType::AssignmentStatement:
                print_assignment_statement(static_cast<const AssignmentStatement*>(node), indent);
                return;
            case ASTNodeType::ForStatement:
                print_for_statement(static_cast<const ForStatement*>(node), indent);
                return;
            case ASTNodeType::IfStatement:
                print_if_statement(static_cast<const IfStatement*>(node), indent);
                return;
            case ASTNodeType::ReturnStatement:
                print_return_statement(static_cast<const ReturnStatement*>(node), indent);
                return;
            case ASTNodeType::VariableDefinitionStatement:
                print_variable_definition_statement(static_cast<const VariableDefinitionStatement*>(node), indent);
                return;
            case ASTNodeType::BooleanLiteralExpression:
                print_boolean_literal_expression(static_cast<const BooleanLiteralExpression*>(node), indent);
                return;
            case ASTNodeType::FloatingPointLiteralExpression:
                print_floating_point_literal_expression(static_cast<const FloatingPointLiteralExpression*>(node), indent);
                return;
            case ASTNodeType::IntegerLiteralExpression:
                print_integer_literal_expression(static_cast<const IntegerLiteralExpression*>(node), indent);
                return;
            case ASTNodeType::StringLiteralExpression:
                print_string_literal_expression(static_cast<const StringLiteralExpression*>(node), indent);
                return;
            case ASTNodeType::BinaryExpression:
                print_binary_expression(static_cast<const BinaryExpression*>(node), indent);
                return;
            case ASTNodeType::CallExpression:
                print_call_expression(static_cast<const CallExpression*>(node), indent);
                return;
            case ASTNodeType::CastExpression:
                print_cast_expression(static_cast<const CastExpression*>(node), indent);
                return;
            case ASTNodeType::MathematicalNegationExpression:
                print_mathematical_negation_expression(static_cast<const MathematicalNegationExpression*>(node), indent);
                return;
            case ASTNodeType::VariableExpression:
                print_variable_expression(static_cast<const VariableExpression*>(node), indent);
                return;
        }

        KPL_ASSERT_UNREACHABLE("Missing ast printer implementation for node of type '{}'", node->node_type);
    }

    void ASTPrinter::print_extern(const Extern* ext, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(ext);
        KPL_ASSERT_NOT_NULLPTR(ext->prototype);
        KPL_ASSERT_THAT(ext->node_type != ASTNodeType::Poison, "Extern must not be poisoned for printing");
        print_node(ext->prototype.get(), "", indent, true);
    }

    void ASTPrinter::print_function(const Function* function, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison, "Function must not be poisoned for printing");
        print_node(function->prototype.get(), "", indent, false);
        print_nodes(function->body, "Body", indent, true);
    }

    void ASTPrinter::print_prototype(const Prototype* prototype, std::string indent) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison, "Prototype must not be posioned for printing");
        std::println("{}{}Linkage: {}", indent, item_prefix, prototype->linkage_type);
        if (prototype->return_type == nullptr) {
            const std::string_view type_name = StringPool::get().lookup(prototype->return_type_id);
            std::println("{}{}Type name: {}", indent, item_prefix, type_name);
        } else {
            std::println("{}{}Type: {}", indent, item_prefix, *prototype->return_type);
        }
        const std::string_view identifier = StringPool::get().lookup(prototype->identifier_id);
        std::println("{}{}Identifier: {}", indent, item_prefix, identifier);

        const Symbol* prototype_symbol = prototype->symbol;
        if (prototype_symbol == nullptr) {
            std::println("{}{}Symbol: {}nullptr{}", indent, item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            const std::string_view prototype_symbol_identifier = StringPool::get().lookup(prototype_symbol->identifier_id);
            std::println("{}{}Symbol: {}", indent, item_prefix, prototype_symbol_identifier);
        }

        std::print("{}{}Parameters: ", indent, last_item_prefix);
        if (prototype->parameter_data.empty()) {
            std::println("{}None{}", ansi_codes::dim, ansi_codes::reset);
        } else {
            std::println();
        }

        indent += space;
        for (size_t i = 0; i < prototype->parameter_data.size(); i++) {
            bool is_last = i == prototype->parameter_data.size() - 1;
            const std::string_view parameter_identifier = StringPool::get().lookup(prototype->parameter_data[i].identifier_id);
            std::string item_indent;
            if (is_last) {
                std::println("{}{}{}:", indent, last_item_prefix, (i + 1));
                item_indent = space;
            } else {
                std::println("{}{}{}:", indent, item_prefix, (i + 1));
                item_indent = vertical_line;
            }

            const Type* parameter_type = prototype->parameter_data[i].type;
            if (parameter_type == nullptr) {
                const std::string_view parameter_type_name = StringPool::get().lookup(prototype->parameter_data[i].type_id);
                std::println("{}{}Type name: {}", indent + item_indent, item_prefix, parameter_type_name);
            } else {
                std::println("{}{}Type: {}", indent + item_indent, item_prefix, *parameter_type);
            }
            std::println("{}{}Identifier: {}", indent + item_indent, item_prefix, parameter_identifier);

            const Symbol* parameter_symbol = prototype->parameter_data[i].symbol;
            if (parameter_symbol == nullptr) {
                std::println("{}{}Symbol: {}nullptr{}", indent + item_indent, last_item_prefix, ansi_codes::dim, ansi_codes::reset);
            } else {
                const std::string_view parameter_symbol_identifier = StringPool::get().lookup(parameter_symbol->identifier_id);
                std::println("{}{}Symbol: {}", indent + item_indent, last_item_prefix, parameter_symbol_identifier);
            }
        }
    }

    void ASTPrinter::print_assignment_statement(const AssignmentStatement* statement, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "AssignmentStatement must not be posioned for printing");
        print_node(statement->variable_expression.get(), "", indent, false);
        print_node(statement->value_expression.get(), "Value: ", indent, true);
    }

    void ASTPrinter::print_for_statement(const ForStatement* statement, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ForStatement must not be poisoned for printing");
        print_node(statement->loop_variable_definition.get(), "", indent, false);
        print_node(statement->end_value.get(), "End: ", indent, false);
        if (statement->step_value == nullptr) {
            std::println("{}{}Step: {}implicit{}", indent, item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            print_node(statement->step_value.get(), "Step: ", indent, false);
        }
        print_nodes(statement->body, "Body", indent, true);
    }

    void ASTPrinter::print_if_statement(const IfStatement* statement, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "IfStatement must not be poisoned for printing");
        print_node(statement->condition.get(), "", indent, false);
        print_nodes(statement->if_body, "If body", indent, false);
        print_nodes(statement->else_body, "Else body", indent, true);
    }

    void ASTPrinter::print_return_statement(const ReturnStatement* statement, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ReturnStatement must not be poisoned for printing");
        if (statement->expression == nullptr) {
            std::println("{}{}Expression: {}nullptr{}", indent, last_item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            print_node(statement->expression.get(), "", indent, true);
        }
    }

    void ASTPrinter::print_variable_definition_statement(const VariableDefinitionStatement* statement, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "VariableDefinitionStatement must not be poisoned for printing");
        if (statement->type == nullptr) {
            const std::string_view type_name = StringPool::get().lookup(statement->type_id);
            std::println("{}{}Type: {}", indent, item_prefix, type_name);
        } else {
            std::println("{}{}Type: {}", indent, item_prefix, *statement->type);
        }
        const std::string_view identifier = StringPool::get().lookup(statement->identifier_id);
        std::println("{}{}Identifier: {}", indent, item_prefix, identifier);
        print_node(statement->assignment_statement.get(), "", indent, true);
    }

    void ASTPrinter::print_boolean_literal_expression(const BooleanLiteralExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BooleanLiteralExpression must not be poisoned for printing");
        std::println("{}{}{}", indent, last_item_prefix, expression->value);
    }

    void ASTPrinter::print_floating_point_literal_expression(const FloatingPointLiteralExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "FloatingPointLiteralExpression must not be poisoned for printing");
        if (expression->target_type == nullptr) {
            std::println("{}{}Type: {}nullptr{}", indent, item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            std::println("{}{}Type: {}", indent, item_prefix, *expression->target_type);
        }
        std::println("{}{}Value: {}", indent, last_item_prefix, expression->value);
    }

    void ASTPrinter::print_integer_literal_expression(const IntegerLiteralExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "IntegerLiteralExpression must not be poisoned for printing");
        if (expression->target_type == nullptr) {
            std::println("{}{}Type: {}nullptr{}", indent, item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            std::println("{}{}Type: {}", indent, item_prefix, *expression->target_type);
        }
        std::println("{}{}Value: {}", indent, last_item_prefix, expression->value);
    }

    void ASTPrinter::print_string_literal_expression(const StringLiteralExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "StringLiteralExpression must not be poisoned for printing");
        const std::string_view str = StringPool::get().lookup(expression->value);
        std::println("{}{}{}", indent, last_item_prefix, str);
    }

    void ASTPrinter::print_binary_expression(const BinaryExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BinaryExpression must not be poisoned for printing");
        std::println("{}{}Operator: {}", indent, item_prefix, expression->operator_type);
        print_node(expression->lhs.get(), "lhs: ", indent, false);
        print_node(expression->rhs.get(), "rhs: ", indent, true);
    }

    void ASTPrinter::print_call_expression(const CallExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CallExpression must not be poisoned for printing");
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        std::println("{}{}Identifier: {}", indent, item_prefix, identifier);

        std::println("{}{}Args: ", indent, item_prefix);
        for (size_t i = 0; i < expression->args.size(); i++) {
            bool is_last = i == expression->args.size() - 1;
            print_node(expression->args[i].get(), "", indent, is_last);
        }

        if (expression->symbol == nullptr) {
            std::println("{}{}Symbol: {}nullptr{}", indent, last_item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            const std::string_view symbol_identifier = StringPool::get().lookup(expression->symbol->identifier_id);
            std::println("{}{}Symbol: {}", indent, last_item_prefix, symbol_identifier);
        }
    }

    void ASTPrinter::print_cast_expression(const CastExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CastExpression must not be poisoned for printing");
        if (expression->target_type == nullptr) {
            const std::string_view target_type_name = StringPool::get().lookup(expression->target_type_id);
            std::println("{}{}Type: {}", indent, item_prefix, target_type_name);
        } else {
            std::println("{}{}Type: {}", indent, item_prefix, *expression->target_type);
        }
        print_node(expression->expression.get(), "", indent, true);
    }

    void ASTPrinter::print_mathematical_negation_expression(const MathematicalNegationExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "MathematicalNegationExpression must not be poisoned for printing");
        print_node(expression->expression.get(), "", indent, true);
    }

    void ASTPrinter::print_variable_expression(const VariableExpression* expression, const std::string& indent) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "VariableExpression must not be poisoned for printing");
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        std::println("{}{}{}", indent, item_prefix, identifier);

        if (expression->symbol == nullptr) {
            std::println("{}{}Symbol: {}nullptr{}", indent, last_item_prefix, ansi_codes::dim, ansi_codes::reset);
        } else {
            const std::string_view symbol_identifier = StringPool::get().lookup(expression->symbol->identifier_id);
            std::println("{}{}Symbol: {}", indent, last_item_prefix, symbol_identifier);
        }
    }

}
