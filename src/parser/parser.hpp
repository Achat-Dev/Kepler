// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/import_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/struct.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_kind.hpp"
#include "lexer/token.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    enum class IdentifierPathParseKind {
        IncludeLastIdentifier,
        ReturnAtLastIdentifier,
    };

    class Parser {
    public:
        Parser(const File* file, const std::vector<Token> tokens, DiagnosticSink& diagnostic_sink)
            : file(file), tokens(std::move(tokens)), diagnostic_sink(diagnostic_sink), current_token(&this->tokens[0]) {
            KPL_ASSERT_NOT_NULLPTR(file);
            KPL_ASSERT_NOT_NULLPTR(current_token);
        }
        AbstractSyntaxTree parse();

    private:
        const File* file;
        const std::vector<Token> tokens;
        DiagnosticSink& diagnostic_sink;
        ArenaAllocator* allocator;
        const Token* current_token;
        size_t current_token_index = 0;

        int get_operator_precedence(OperatorKind operator_kind) const;
        void next_token(bool skip_newline);
        void previous_token(bool skip_newline);
        TokenKind peek_next_token_kind(size_t lookahead, bool skip_newline) const;
        void jump_to_token(size_t index);
        std::optional<IdentifierPath> parse_identifier_path(TokenKind delimiter_token,
            IdentifierPathParseKind path_parse_kind,
            const std::string& diagnostic_ending);
        IdentifierPath identifier_id_to_path(StringId identifier_id, SourceLocation source_location) const;

        // Top level
        ModuleStatement* parse_module();
        ImportStatement* parse_import();
        ExportableNode* parse_export();
        Extern* parse_extern(LinkageKind linkage_kind);
        Prototype* parse_prototype();
        Struct* parse_struct(LinkageKind linkage_kind);
        ExportableNode* parse_top_level_identifier(LinkageKind linkage_kind);
        Function* parse_function(LinkageKind linkage_kind);

        // Main body nodes
        ASTNode* parse_statement();
        Expression* parse_expression();

        // Expressions
        Expression* parse_binary_expression_rhs(Expression* lhs, int expression_precedence);
        Expression* parse_primary();
        Expression* parse_literal();
        Expression* parse_parenthesis();
        Expression* parse_identifier_expression();
        CallExpression* parse_call(std::optional<IdentifierPath> module_path, IdentifierPath callee_identifier);
        MathematicalNegationExpression* parse_negative();
        ObjectInitializerExpression* parse_object_initializer();

        // Statements
        ASTNode* parse_identifier_statement();
        AssignmentStatement* parse_assignment(IdentifierPath identifier_path);
        IfStatement* parse_if();
        ForStatement* parse_for();
        ForStatement* create_for_statement(const Token* variable_type_token,
            const Token* variable_identifier_token,
            Expression* start_value,
            Expression* end_value,
            Expression* step_value,
            SourceLocation for_source_location);
        void recover_for_definition_and_parse_body(SourceLocation source_location);
        ReturnStatement* parse_return();
        VariableDefinitionStatement* parse_variable_definition();

        template <TokenKind... T>
        struct SynchronizationSet {};

        template <TokenKind... Sync, TokenKind... Consume>
        void recover(SynchronizationSet<Sync...> synchronisation_tokens, SynchronizationSet<Consume...> consume_tokens) {
            while (!synchronisation_tokens_contains<Sync...>(current_token->token_kind) && current_token->token_kind != TokenKind::EndOfFile) {
                next_token(false);
            }
            if (synchronisation_tokens_contains<Consume...>(current_token->token_kind)) {
                next_token(true);
            }
        }

        template <TokenKind... T>
        bool synchronisation_tokens_contains(TokenKind token_kind) {
            return ((token_kind == T) || ...);
        }

        template <TokenKind... T>
        std::optional<std::vector<ASTNode*>> parse_body(std::string diagnostic_message_on_end, SourceLocation source_location) {
            std::vector<ASTNode*> body;
            while (((current_token->token_kind != T) && ...)) {
                if (current_token->token_kind == TokenKind::EndOfFile) {
                    diagnostic_sink.report(DiagnosticCode::MissingEndKeyword, std::move(diagnostic_message_on_end), source_location);
                    return std::nullopt;
                }

                ASTNode* statement = parse_statement();
                if (statement) {
                    body.push_back(statement);
                }
            }
            return body;
        }
    };

}
