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
#include "ast/function.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <memory>
#include <vector>

namespace kepler {

    class TypeCheckPass : ASTPass<void> {
    public:
        struct TypeCheckResult {
            enum class Status {
                RequestFulfilled,
                PoisonedWithDiagnostic,
                PosionedWithoutDiagnostic,
            };

            Status status;
            DataTypeKind data_type;

            bool is_poisoned() const {
                return status == Status::PoisonedWithDiagnostic || status == Status::PosionedWithoutDiagnostic;
            }
        };

        TypeCheckPass(AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, const SymbolTable& symbol_table)
            : ASTPass(ast), diagnostic_sink(diagnostic_sink), symbol_table(symbol_table) {}
        void run() override;

    private:
        DataTypeKind current_function_return_type;
        DiagnosticSink& diagnostic_sink;
        const SymbolTable& symbol_table;

        bool is_boolean_operator(OperatorType type) const;

        TypeCheckResult typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes);
        TypeCheckResult typecheck_node(ASTNode* node, DataTypeKind requested_data_type);
        void typecheck_function(const Function* function);
        TypeCheckResult typecheck_assignment_statement(AssignmentStatement* statement);
        TypeCheckResult typecheck_for_statement(ForStatement* statement);
        TypeCheckResult typecheck_if_statement(IfStatement* statement);
        TypeCheckResult typecheck_return_statement(ReturnStatement* statement);
        TypeCheckResult typecheck_variable_definition_statement(VariableDefinitionStatement* statement);
        TypeCheckResult typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, DataTypeKind requested_data_type) const;
        TypeCheckResult typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, DataTypeKind requested_data_type) const;
        TypeCheckResult typecheck_integer_literal_expression(IntegerLiteralExpression* expression, DataTypeKind requested_data_type) const;
        TypeCheckResult typecheck_string_literal_expression(StringLiteralExpression* expression, DataTypeKind requested_data_type) const;
        TypeCheckResult typecheck_binary_expression(BinaryExpression* expression, DataTypeKind requested_data_type);
        TypeCheckResult typecheck_call_expression(CallExpression* expression, DataTypeKind requested_data_type);
        TypeCheckResult typecheck_cast_expression(CastExpression* expression, DataTypeKind requested_data_type) const;
        TypeCheckResult typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, DataTypeKind requested_data_type);
        TypeCheckResult typecheck_variable_expression(VariableExpression* expression, DataTypeKind requested_data_type) const;
    };
}
