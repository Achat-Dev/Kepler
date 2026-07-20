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
        void resolve_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) const;
        void resolve_node(const ASTNode* node) const;
        void resolve_extern(const Extern* ext) const;
        void resolve_function(const Function* function) const;
        void resolve_prototype(const Prototype* prototype) const;
        void resolve_assignment_statement(const AssignmentStatement* statement) const;
        void resolve_for_statement(const ForStatement* statement) const;
        void resolve_if_statement(const IfStatement* statement) const;
        void resolve_return_statement(const ReturnStatement* statement) const;
        void resolve_variable_definition_statement(const VariableDefinitionStatement* statement) const;
        void resolve_binary_expression(const BinaryExpression* expression) const;
        void resolve_call_expression(const CallExpression* expression) const;
        void resolve_cast_expression(const CastExpression* expression) const;
        void resolve_mathematical_negation_expression(const MathematicalNegationExpression* expression) const;
        void resolve_variable_expression(const VariableExpression* expression) const;
    };

}
