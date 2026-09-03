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
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/source_location.hpp"
#include "io/file.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    // TODO (improvement): Replacing unique_ptrs with raw pointers and an arena allocator could improve performance
    class Parser {
    public:
        Parser(const std::vector<Token>& tokens, const File& file, DiagnosticSink& diagnostic_sink, TypeTable& type_table)
            : tokens(tokens), file(file), diagnostic_sink(diagnostic_sink), type_table(type_table), current_token(&tokens[0]) {}
        AbstractSyntaxTree parse();

    private:
        const std::vector<Token>& tokens;
        const File& file;
        DiagnosticSink& diagnostic_sink;
        const TypeTable& type_table;
        const Token* current_token;
        std::optional<StringId> current_function_return_type_id = std::nullopt;
        size_t current_token_index = 0;

        int get_operator_precedence(OperatorType operator_type) const;
        void next_token(bool skip_newline);
        void previous_token(bool skip_newline);
        void jump_to_token(size_t index);

        // Top level
        std::unique_ptr<Extern> parse_extern();
        std::unique_ptr<Prototype> parse_prototype(Prototype::LinkageType linkage_type);
        std::unique_ptr<ASTNode> parse_top_level_type();
        std::unique_ptr<Function> parse_function();

        // Main body nodes
        std::unique_ptr<ASTNode> parse_statement();
        std::unique_ptr<Expression> parse_expression();

        // Expressions
        std::unique_ptr<Expression> parse_binary_expression_rhs(std::unique_ptr<Expression> lhs, int expression_precedence);
        std::unique_ptr<Expression> parse_primary();
        std::unique_ptr<Expression> parse_literal();
        std::unique_ptr<Expression> parse_parenthesis();
        std::unique_ptr<Expression> parse_identifier();
        std::unique_ptr<CallExpression> parse_call(const Token* identifier_token);
        std::unique_ptr<MathematicalNegationExpression> parse_negative();
        std::unique_ptr<CastExpression> parse_cast();

        // Statements
        std::unique_ptr<AssignmentStatement> parse_assignment(const Token* identifier_token);
        std::unique_ptr<IfStatement> parse_if();
        std::unique_ptr<ForStatement> parse_for();
        std::unique_ptr<ForStatement> create_for_statement(StringId identifier_id,
            const Token* variable_type_token,
            std::unique_ptr<Expression> start_value,
            std::unique_ptr<Expression> end_value,
            std::unique_ptr<Expression> step_value,
            SourceLocation for_source_location);
        void recover_for_definition_and_parse_body(SourceLocation source_location);
        std::unique_ptr<ReturnStatement> parse_return();
        std::unique_ptr<VariableDefinitionStatement> parse_variable_definition();

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
        std::optional<std::vector<std::unique_ptr<ASTNode>>> parse_body(std::string diagnostic_message_on_end, SourceLocation source_location) {
            std::vector<std::unique_ptr<ASTNode>> body;
            while (((current_token->type != T) && ...)) {
                if (current_token->type == TokenType::EndOfFile) {
                    diagnostic_sink.report(DiagnosticCode::MissingEndKeyword, std::move(diagnostic_message_on_end), source_location);
                    return std::nullopt;
                }

                std::unique_ptr<ASTNode> statement = parse_statement();
                if (statement) {
                    body.push_back(std::move(statement));
                }
            }
            return body;
        }
    };

}
