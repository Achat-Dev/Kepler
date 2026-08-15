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
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include <memory>
#include <vector>

namespace kepler {

    struct TypeCheckResult {
        enum class Status {
            RequestFulfilled,
            PoisonedWithDiagnostic,
            PoisonedWithoutDiagnostic,
        };

        Status status;
        Type* type;

        bool is_poisoned() const {
            return status == Status::PoisonedWithDiagnostic || status == Status::PoisonedWithoutDiagnostic;
        }
    };

    class TypeCheckPass : ASTPass<void> {
    public:
        TypeCheckPass(AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, const TypeTable& type_table)
            : ASTPass(ast), diagnostic_sink(diagnostic_sink), type_table(type_table) {}
        void run() override;

    private:
        Type* current_function_return_type = nullptr;
        DiagnosticSink& diagnostic_sink;
        const TypeTable& type_table;

        bool is_boolean_operator(OperatorType type) const;

        TypeCheckResult typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes);
        TypeCheckResult typecheck_node(ASTNode* node, Type* requested_type);
        void typecheck_function(Function* function);
        TypeCheckResult typecheck_assignment_statement(AssignmentStatement* statement);
        TypeCheckResult typecheck_for_statement(ForStatement* statement);
        TypeCheckResult typecheck_body_and_poison_for_statement(ForStatement* statement);
        TypeCheckResult typecheck_if_statement(IfStatement* statement);
        TypeCheckResult typecheck_return_statement(ReturnStatement* statement);
        TypeCheckResult typecheck_variable_definition_statement(VariableDefinitionStatement* statement);
        TypeCheckResult typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, const Type* requested_type) const;
        TypeCheckResult typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, Type* requested_type) const;
        TypeCheckResult typecheck_integer_literal_expression(IntegerLiteralExpression* expression, Type* requested_type) const;
        TypeCheckResult typecheck_string_literal_expression(StringLiteralExpression* expression, const Type* requested_type) const;
        TypeCheckResult typecheck_binary_expression(BinaryExpression* expression, Type* requested_type);
        TypeCheckResult typecheck_binary_expression_side(BinaryExpression* binary_expression, Expression* side_expression, Type* requested_type);
        TypeCheckResult typecheck_call_expression(CallExpression* expression, const Type* requested_type);
        TypeCheckResult typecheck_cast_expression(CastExpression* expression, const Type* requested_type);
        TypeCheckResult typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, Type* requested_type);
        TypeCheckResult typecheck_variable_expression(VariableExpression* expression, const Type* requested_type) const;
    };
}
