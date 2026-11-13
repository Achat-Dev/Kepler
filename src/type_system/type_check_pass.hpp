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
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/function.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include <cstdint>
#include <expected>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct TypeCheckResult {
        enum class Status {
            RequestFulfilled,
            PoisonedWithDiagnostic,
            PoisonedWithoutDiagnostic,
        };

        Status status;
        TypeId type_id;

        bool is_poisoned() const {
            return status == Status::PoisonedWithDiagnostic || status == Status::PoisonedWithoutDiagnostic;
        }
    };

    // Important: The order of these determines the priority of the match kind
    // Lower means hihger priority
    enum class MatchingCallCandidateKind {
        ExactMatch,
        ExactMatchWithAdjustments,
        VariadicMatch,
        VariadicMatchWithAdjustments,
    };

    struct MatchingCallCandidate {
        SymbolId symbol_id;
        MatchingCallCandidateKind match_kind;
        std::unordered_map<uint32_t, TypeId> literal_type_ids_to_update;
    };

    class TypeCheckPass : ASTPass<void> {
    public:
        TypeCheckPass(DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table)
            : diagnostic_sink(diagnostic_sink), symbol_table(symbol_table), type_table(type_table) {}
        void run(std::vector<AbstractSyntaxTree>& asts) override;

    private:
        DiagnosticSink& diagnostic_sink;
        SymbolTable& symbol_table;
        TypeTable& type_table;
        ModuleId module_id;
        TypeId current_function_return_type_id;

        bool is_boolean_operator(OperatorType type) const;
        bool is_number_literal_expression(const Expression* expression) const;

        TypeCheckResult typecheck_nodes(const std::vector<ASTNode*>& nodes);
        TypeCheckResult typecheck_node(ASTNode* node, TypeId requested_type_id);
        void typecheck_function(Function* function);
        TypeCheckResult typecheck_assignment_statement(AssignmentStatement* statement);
        TypeCheckResult typecheck_for_statement(ForStatement* statement);
        TypeCheckResult typecheck_body_and_poison_for_statement(ForStatement* statement);
        TypeCheckResult typecheck_if_statement(IfStatement* statement);
        TypeCheckResult typecheck_return_statement(ReturnStatement* statement);
        TypeCheckResult typecheck_variable_definition_statement(VariableDefinitionStatement* statement);
        TypeCheckResult typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, TypeId requested_type_id) const;
        TypeCheckResult typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, TypeId requested_type_id) const;
        TypeCheckResult typecheck_integer_literal_expression(IntegerLiteralExpression* expression, TypeId requested_type_id, bool is_negative) const;
        TypeCheckResult typecheck_string_literal_expression(StringLiteralExpression* expression, TypeId requested_type_id) const;
        TypeCheckResult typecheck_binary_expression(BinaryExpression* expression, TypeId requested_type_id);
        TypeCheckResult typecheck_call_expression(CallExpression* expression, TypeId requested_type_id);
        std::expected<SymbolId, Diagnostic> find_matching_prototype_symbol(TypeId return_type_id,
            const std::vector<SymbolId>& candidate_symbol_ids,
            const std::vector<Expression*>& arguments,
            const std::vector<TypeId>& argument_type_ids);
        // IMPORTANT: This method expects the candidate_symbol_ids to have already been filtered based on their names.
        // In other words, this method only checks the arguments of the candidates and not their names
        std::vector<MatchingCallCandidate> find_matching_call_candidates(const std::vector<SymbolId>& candidate_symbol_ids,
            const std::vector<TypeId>& argument_type_ids,
            const std::vector<Expression*>& argument_expressions) const;
        void update_call_argument_target_types(const std::unordered_map<uint32_t, TypeId>& argument_indices_to_update,
            const std::vector<Expression*>& argument_expressions);
        void set_matching_call_candidate(CallExpression* expression, SymbolId candidate_symbol_id, const std::vector<TypeId>& argument_type_ids);
        TypeCheckResult typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, TypeId requested_type_id);
        TypeCheckResult typecheck_member_expression(MemberExpression* expression, TypeId requested_type_id) const;
        TypeCheckResult typecheck_object_initializer_expression(ObjectInitializerExpression* expression, TypeId requested_type_id);
        TypeCheckResult typecheck_variable_expression(VariableExpression* expression, TypeId requested_type_id) const;
    };
}
