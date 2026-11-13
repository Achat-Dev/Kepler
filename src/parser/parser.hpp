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
#include "lexer/operator_type.hpp"
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

        int get_operator_precedence(OperatorType operator_type) const;
        void next_token(bool skip_newline);
        void previous_token(bool skip_newline);
        TokenType peek_next_token_type(size_t lookahead, bool skip_newline) const;
        void jump_to_token(size_t index);
        std::optional<IdentifierPath> parse_identifier_path(TokenType delimiter_token,
            IdentifierPathParseKind path_return_kind,
            const std::string& diagnostic_ending);
        IdentifierPath identifier_id_to_path(StringId identifier_id, SourceLocation source_location) const;

        // Top level
        ModuleStatement* parse_module();
        ImportStatement* parse_import();
        ExportableNode* parse_export();
        Extern* parse_extern(LinkageType linkage_type);
        Prototype* parse_prototype();
        Struct* parse_struct(LinkageType linkage_type);
        ExportableNode* parse_top_level_identifier(LinkageType linkage_type);
        Function* parse_function(LinkageType linkage_type);

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

        template <TokenType... T>
        struct SynchronizationSet {};

        template <TokenType... Sync, TokenType... Consume>
        void recover(SynchronizationSet<Sync...> synchronisation_tokens, SynchronizationSet<Consume...> consume_tokens) {
            while (!synchronisation_tokens_contains<Sync...>(current_token->type) && current_token->type != TokenType::EndOfFile) {
                next_token(false);
            }
            if (synchronisation_tokens_contains<Consume...>(current_token->type)) {
                next_token(true);
            }
        }

        template <TokenType... T>
        bool synchronisation_tokens_contains(TokenType token_type) {
            return ((token_type == T) || ...);
        }

        template <TokenType... T>
        std::optional<std::vector<ASTNode*>> parse_body(std::string diagnostic_message_on_end, SourceLocation source_location) {
            std::vector<ASTNode*> body;
            while (((current_token->type != T) && ...)) {
                if (current_token->type == TokenType::EndOfFile) {
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
