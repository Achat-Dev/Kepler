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
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "ast/struct.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <expected>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct NameResolutionResult {
        bool poisoned = false;
    };

    struct OperatorSignatureData {
        TypeId return_type_id;
        StringId identifier_id;
        uint32_t parameter_count;
    };

    class NameResolutionPass : public ASTPass<void> {
    public:
        NameResolutionPass(DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table);
        void run(std::vector<AbstractSyntaxTree>& asts) override;

    private:
        DiagnosticSink& diagnostic_sink;
        SymbolTable& symbol_table;
        TypeTable& type_table;
        ModuleId module_id;
        // We can't use builtin_utils for this because we map identifier ids to symbols here instead of type kinds
        std::unordered_map<StringId, SymbolId> builtin_type_symbol_ids;

        void register_builtin_type_symbol(TypeKind type_kind);
        void create_struct_symbols_and_types(const std::vector<Struct*>& struct_nodes);
        void create_struct_symbol(Struct* struct_node);
        void create_prototype_symbols(const std::vector<ASTNode*>& top_level_nodes);
        NameResolutionResult create_prototype_symbol(Prototype* prototype, LinkageKind linkage_type) const;
        NameResolutionResult ensure_operator_signatures(Prototype* prototype,
            TypeId object_type_id,
            StringId method_identifier_id,
            uint32_t parameter_count) const;
        void ensure_operator_signature(Prototype* prototype,
            const OperatorSignatureData& expected_signature,
            const OperatorSignatureData& actual_signature) const;

        void create_struct_members(const std::vector<Struct*>& struct_nodes);
        NameResolutionResult resolve_nodes(std::vector<ASTNode*>& nodes) const;
        NameResolutionResult resolve_node(ASTNode* node) const;
        void resolve_extern(Extern* ext) const;
        void resolve_function(Function* function) const;
        NameResolutionResult resolve_prototype(Prototype* prototype) const;
        NameResolutionResult resolve_assignment_statement(AssignmentStatement* statement, SymbolId variable_symbol_id) const;
        NameResolutionResult resolve_for_statement(ForStatement* statement) const;
        NameResolutionResult resolve_if_statement(IfStatement* statement) const;
        NameResolutionResult resolve_return_statement(ReturnStatement* statement) const;
        NameResolutionResult resolve_variable_definition_statement(VariableDefinitionStatement* statement) const;
        NameResolutionResult resolve_binary_expression(BinaryExpression* expression) const;
        NameResolutionResult resolve_call_expression(CallExpression* expression) const;
        NameResolutionResult resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const;
        NameResolutionResult resolve_member_expression(MemberExpression* expression) const;
        NameResolutionResult resolve_object_initializer_expression(ObjectInitializerExpression* expression) const;
        NameResolutionResult resolve_variable_expression(VariableExpression* expression, SymbolId variable_symbol_id) const;

        std::expected<TypeSymbol*, Diagnostic> resolve_type_identifier_to_type_symbol(StringId type_identifier_id) const;
    };

}
