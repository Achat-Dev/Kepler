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
            std::print("{}{}{}: ", indent, last_item, label);
            indent += space;
        } else {
            std::print("{}{}{}: ", indent, item, label);
            indent += vertical;
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
        if (node == nullptr) {
            return;
        }

        if (node->node_type == ASTNodeType::Poison) {
            std::print(ansi_codes::magenta);
        }

        std::print("{}", indent);
        if (is_last) {
            std::print(last_item);
            indent += space;
        } else {
            std::print(item);
            indent += vertical;
        }
        std::print("{}{}{}", prefix, node->node_type, ansi_codes::reset);

        switch (node->node_type) {
            case ASTNodeType::Poison:
                return;
            case ASTNodeType::Extern:
                print_extern(static_cast<const Extern*>(node), indent);
                break;
            case ASTNodeType::Function:
                print_function(static_cast<const Function*>(node), indent);
                break;
            case ASTNodeType::Prototype:
                print_prototype(static_cast<const Prototype*>(node), indent);
                break;
            case ASTNodeType::AssignmentStatement:
                print_assignment_statement(static_cast<const AssignmentStatement*>(node), indent);
                break;
            case ASTNodeType::ForStatement:
                print_for_statement(static_cast<const ForStatement*>(node), indent);
                break;
            case ASTNodeType::IfStatement:
                print_if_statement(static_cast<const IfStatement*>(node), indent);
                break;
            case ASTNodeType::ReturnStatement:
                print_return_statement(static_cast<const ReturnStatement*>(node), indent);
                break;
            case ASTNodeType::VariableDefinitionStatement:
                print_variable_definition_statement(static_cast<const VariableDefinitionStatement*>(node), indent);
                break;
            case ASTNodeType::BooleanLiteralExpression:
                print_boolean_literal_expression(static_cast<const BooleanLiteralExpression*>(node), indent);
                break;
            case ASTNodeType::FloatingPointLiteralExpression:
                print_floating_point_literal_expression(static_cast<const FloatingPointLiteralExpression*>(node), indent);
                break;
            case ASTNodeType::IntegerLiteralExpression:
                print_integer_literal_expression(static_cast<const IntegerLiteralExpression*>(node), indent);
                break;
            case ASTNodeType::StringLiteralExpression:
                print_string_literal_expression(static_cast<const StringLiteralExpression*>(node), indent);
                break;
            case ASTNodeType::BinaryExpression:
                print_binary_expression(static_cast<const BinaryExpression*>(node), indent);
                break;
            case ASTNodeType::CallExpression:
                print_call_expression(static_cast<const CallExpression*>(node), indent);
                break;
            case ASTNodeType::CastExpression:
                print_cast_expression(static_cast<const CastExpression*>(node), indent);
                break;
            case ASTNodeType::MathematicalNegationExpression:
                print_mathematical_negation_expression(static_cast<const MathematicalNegationExpression*>(node), indent);
                break;
            case ASTNodeType::VariableExpression:
                print_variable_expression(static_cast<const VariableExpression*>(node), indent);
                break;
        }

        KPL_ASSERT_UNREACHABLE("Missing ast printer implementation for node of type '{}'", node->node_type);
    }

    void ASTPrinter::print_extern(const Extern* ext, const std::string& indent) const {
        print_node(ext->prototype.get(), "", indent, true);
    }

    void ASTPrinter::print_function(const Function* function, const std::string& indent) const {
        print_node(function->prototype.get(), "", indent, false);
        print_nodes(function->body, "Body", indent, true);
    }

    void ASTPrinter::print_prototype(const Prototype* prototype, std::string indent) const {
        std::println("{}{}Linkage: {}", indent, item, prototype->linkage_type);
        if (prototype->return_type == nullptr) {
            const std::string_view type_name = StringPool::get().lookup(prototype->return_type_id);
            std::println("{}{}Type: {}", indent, item, type_name);
        } else {
            std::println("{}{}Type: {}", indent, item, *prototype->return_type);
        }
        const std::string_view identifier = StringPool::get().lookup(prototype->identifier_id);
        std::println("{}{}Identifier: {}", indent, item, identifier);
        std::print("{}{}Parameters: ", indent, last_item);

        if (prototype->parameter_data.empty()) {
            std::println("{}None{}", ansi_codes::dim, ansi_codes::reset);
        } else {
            std::println();
        }

        indent += space;
        for (size_t i = 0; i < prototype->parameter_data.size(); i++) {
            bool is_last = i == prototype->parameter_data.size() - 1;
            const std::string_view parameter_identifier = StringPool::get().lookup(prototype->parameter_data[i].identifier_id);
            Type* parameter_type = prototype->parameter_data[i].type;
            std::string item_indent;
            if (is_last) {
                std::println("{}{}{}:", indent, last_item, (i + 1));
                item_indent = space;
            } else {
                std::println("{}{}{}:", indent, item, (i + 1));
                item_indent = vertical;
            }

            if (parameter_type == nullptr) {
                const std::string_view parameter_type_name = StringPool::get().lookup(prototype->parameter_data[i].type_id);
                std::println("{}{}Type: {}", indent + item_indent, item, parameter_type_name);
            } else {
                std::println("{}{}Type: {}", indent + item_indent, item, *parameter_type);
            }
            std::println("{}{}Identifier: {}", indent + item_indent, last_item, parameter_identifier);
        }
    }

    void ASTPrinter::print_assignment_statement(const AssignmentStatement* statement, const std::string& indent) const {
        print_node(statement->variable_expression.get(), "", indent, false);
        print_node(statement->value_expression.get(), "Value: ", indent, true);
    }

    void ASTPrinter::print_for_statement(const ForStatement* statement, const std::string& indent) const {
        print_node(statement->loop_variable_definition.get(), "", indent, false);
        print_node(statement->end_value.get(), "End: ", indent, false);
        print_node(statement->step_value.get(), "Step: ", indent, false);
        print_nodes(statement->body, "Body", indent, true);
    }

    void ASTPrinter::print_if_statement(const IfStatement* statement, const std::string& indent) const {
        print_node(statement->condition.get(), "", indent, false);
        print_nodes(statement->if_body, "If body", indent, false);
        print_nodes(statement->else_body, "Else body", indent, true);
    }

    void ASTPrinter::print_return_statement(const ReturnStatement* statement, const std::string& indent) const {
        print_node(statement->expression.get(), "", indent, true);
    }

    void ASTPrinter::print_variable_definition_statement(const VariableDefinitionStatement* statement, const std::string& indent) const {
        if (statement->type == nullptr) {
            const std::string_view type_name = StringPool::get().lookup(statement->type_id);
            std::println("{}{}Type: {}", indent, item, type_name);
        } else {
            std::println("{}{}Type: {}", indent, item, *statement->type);
        }
        const std::string_view identifier = StringPool::get().lookup(statement->identifier_id);
        std::println("{}{}Identifier: {}", indent, item, identifier);
        print_node(statement->assignment_statement.get(), "", indent, true);
    }

    void ASTPrinter::print_boolean_literal_expression(const BooleanLiteralExpression* expression, const std::string& indent) const {
        std::println("{}{}{}", indent, last_item, expression->value);
    }

    void ASTPrinter::print_floating_point_literal_expression(const FloatingPointLiteralExpression* expression, const std::string& indent) const {
        std::println("{}{}{}", indent, last_item, expression->value);
    }

    void ASTPrinter::print_integer_literal_expression(const IntegerLiteralExpression* expression, const std::string& indent) const {
        std::println("{}{}{}", indent, last_item, expression->value);
    }

    void ASTPrinter::print_string_literal_expression(const StringLiteralExpression* expression, const std::string& indent) const {
        const std::string_view str = StringPool::get().lookup(expression->value);
        std::println("{}{}{}", indent, last_item, str);
    }

    void ASTPrinter::print_binary_expression(const BinaryExpression* expression, const std::string& indent) const {
        std::println("{}{}Operator: {}", indent, item, expression->operator_type);
        print_node(expression->lhs.get(), "lhs: ", indent, false);
        print_node(expression->rhs.get(), "rhs: ", indent, true);
    }

    void ASTPrinter::print_call_expression(const CallExpression* expression, const std::string& indent) const {
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        std::println("{}{}Identifier: {}", indent, item, identifier);
        std::println("{}{}Args: ", indent, last_item);

        for (size_t i = 0; i < expression->args.size(); i++) {
            bool is_last = i == expression->args.size() - 1;
            print_node(expression->args[i].get(), "", indent, is_last);
        }
    }

    void ASTPrinter::print_cast_expression(const CastExpression* expression, const std::string& indent) const {
        if (expression->target_type == nullptr) {
            const std::string_view target_type_name = StringPool::get().lookup(expression->target_type_id);
            std::println("{}{}Type: {}", indent, item, target_type_name);
        } else {
            std::println("{}{}Type: {}", indent, item, *expression->target_type);
        }
        print_node(expression->expression.get(), "", indent, true);
    }

    void ASTPrinter::print_mathematical_negation_expression(const MathematicalNegationExpression* expression, const std::string& indent) const {
        print_node(expression->expression.get(), "", indent, true);
    }

    void ASTPrinter::print_variable_expression(const VariableExpression* expression, const std::string& indent) const {
        const std::string_view identifier = StringPool::get().lookup(expression->identifier_id);
        std::println("{}{}{}", indent, last_item, identifier);
    }

}
