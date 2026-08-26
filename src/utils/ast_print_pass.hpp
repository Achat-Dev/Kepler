// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_pass.hpp"
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
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include <memory>
#include <string>
#include <vector>

namespace kepler {

    class ASTPrintPass : ASTPass<void> {
    public:
        explicit ASTPrintPass(AbstractSyntaxTree& ast) : ASTPass(ast) {}
        void run() override;

    private:
        static constexpr char space[] = "   ";
        static constexpr char vertical_line[] = " \u2502 ";
        static constexpr char item_prefix[] = " \u251C\u2500 ";
        static constexpr char last_item_prefix[] = " \u2514\u2500 ";

        // TODO (improvement): Some of the const string& arguments could be turned into string_views or const char*, which could avoid some memory overhead
        void print_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes, const std::string& label, std::string indent, bool is_last) const;
        void print_node(const ASTNode* node, const std::string& prefix, std::string indent, bool is_last) const;
        void print_extern(const Extern* ext, const std::string& indent) const;
        void print_function(const Function* function, const std::string& indent) const;
        void print_prototype(const Prototype* prototype, std::string indent) const;
        void print_assignment_statement(const AssignmentStatement* statement, const std::string& indent) const;
        void print_for_statement(const ForStatement* statement, const std::string& indent) const;
        void print_if_statement(const IfStatement* statement, const std::string& indent) const;
        void print_return_statement(const ReturnStatement* statement, const std::string& indent) const;
        void print_variable_definition_statement(const VariableDefinitionStatement* statement, const std::string& indent) const;
        void print_boolean_literal_expression(const BooleanLiteralExpression* expression, const std::string& indent) const;
        void print_floating_point_literal_expression(const FloatingPointLiteralExpression* expression, const std::string& indent) const;
        void print_integer_literal_expression(const IntegerLiteralExpression* expression, const std::string& indent) const;
        void print_string_literal_expression(const StringLiteralExpression* expression, const std::string& indent) const;
        void print_binary_expression(const BinaryExpression* expression, const std::string& indent) const;
        void print_call_expression(const CallExpression* expression, const std::string& indent) const;
        void print_cast_expression(const CastExpression* expression, const std::string& indent) const;
        void print_mathematical_negation_expression(const MathematicalNegationExpression* expression, const std::string& indent) const;
        void print_variable_expression(const VariableExpression* expression, const std::string& indent) const;
        void print_body(const NodeBody& body, const std::string& prefix, std::string indent, bool is_last) const;
    };

}
