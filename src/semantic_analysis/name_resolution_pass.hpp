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
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"

namespace kepler {

    struct NameResolutionResult {
        bool poisoned = false;
    };

    class NameResolutionPass : public ASTPass<void> {
    public:
        NameResolutionPass(AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table)
            : ASTPass(ast), diagnostic_sink(diagnostic_sink), symbol_table(symbol_table), type_table(type_table) {}
        void run() override;

    private:
        DiagnosticSink& diagnostic_sink;
        SymbolTable& symbol_table;
        TypeTable& type_table;

        void collect_prototype_symbols() const;
        void create_prototype_symbol(Prototype* prototype) const;
        NameResolutionResult resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const;
        NameResolutionResult resolve_node(ASTNode* node) const;
        void resolve_extern(Extern* ext) const;
        void resolve_function(Function* function) const;
        NameResolutionResult resolve_prototype(Prototype* prototype) const;
        NameResolutionResult resolve_assignment_statement(AssignmentStatement* statement) const;
        NameResolutionResult resolve_for_statement(ForStatement* statement) const;
        NameResolutionResult resolve_if_statement(IfStatement* statement) const;
        NameResolutionResult resolve_return_statement(ReturnStatement* statement) const;
        NameResolutionResult resolve_variable_definition_statement(VariableDefinitionStatement* statement) const;
        NameResolutionResult resolve_binary_expression(BinaryExpression* expression) const;
        NameResolutionResult resolve_call_expression(CallExpression* expression) const;
        NameResolutionResult resolve_cast_expression(CastExpression* expression) const;
        NameResolutionResult resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const;
        NameResolutionResult resolve_variable_expression(VariableExpression* expression) const;

        void report_unknown_type(StringId type_id, SourceLocation source_location) const;
    };

}
