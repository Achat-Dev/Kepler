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
#include "diagnostics/diagnostic_sink.hpp"
#include "semantic_analysis/symbol_table.hpp"

namespace kepler {

    class NameResolutionPass : public ASTPass<void> {
    public:
        NameResolutionPass(const AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table)
            : ASTPass(ast, diagnostic_sink), symbol_table(symbol_table) {}
        void run() override;

    private:
        SymbolTable& symbol_table;

        void collect_prototype_symbols() const;
        void create_prototype_symbol(const Prototype* prototype) const;
        void analyse_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) const;
        void analyse_node(const ASTNode* node) const;
        void analyse_extern(const Extern* ext) const;
        void analyse_function(const Function* function) const;
        void analyse_prototype(const Prototype* prototype) const;
        void analyse_assignment_statement(const AssignmentStatement* statement) const;
        void analyse_for_statement(const ForStatement* statement) const;
        void analyse_if_statement(const IfStatement* statement) const;
        void analyse_return_statement(const ReturnStatement* statement) const;
        void analyse_variable_definition_statement(const VariableDefinitionStatement* statement) const;
        void analyse_binary_expression(const BinaryExpression* expression) const;
        void analyse_call_expression(const CallExpression* expression) const;
        void analyse_cast_expression(const CastExpression* expression) const;
        void analyse_mathematical_negation_expression(const MathematicalNegationExpression* expression) const;
        void analyse_variable_expression(const VariableExpression* expression) const;
    };

}
