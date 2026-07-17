// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
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
#include "lexer/token_type.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kepler::parser {

    // TODO: Replace shared_ptr with raw pointers and an arena allocator once the architecture rework is finished
    class Parser {
    public:
        Parser(const std::vector<lexer::Token>& tokens, const io::File& file, diagnostics::DiagnosticSink& diagnostic_sink)
            : tokens(tokens), file(file), diagnostic_sink(diagnostic_sink), current_token(&tokens[0]) {}
        ast::AbstractSyntaxTree parse();

    private:
        const std::vector<lexer::Token>& tokens;
        const io::File& file;
        diagnostics::DiagnosticSink& diagnostic_sink;
        const lexer::Token* current_token;
        size_t current_token_index = 0;

        int get_operator_precedence(lexer::OperatorType operator_type) const;
        void next_token(bool skip_newline);
        void previous_token(bool skip_newline);
        void jump_to_token(size_t index);

        // Top level
        std::unique_ptr<ast::Extern> parse_extern();
        std::unique_ptr<ast::Prototype> parse_prototype(ast::Prototype::LinkageType linkage_type);
        std::unique_ptr<ast::ASTNode> parse_top_level_data_type();
        std::unique_ptr<ast::Function> parse_function(type_system::DataTypeKind return_type);

        // Main body nodes
        std::unique_ptr<ast::ASTNode> parse_statement();
        std::unique_ptr<ast::Expression> parse_expression();

        // Expressions
        std::unique_ptr<ast::Expression> parse_binary_expression_rhs(std::unique_ptr<ast::Expression> lhs_expression, int expression_precedence);
        std::unique_ptr<ast::Expression> parse_primary();
        std::unique_ptr<ast::Expression> parse_literal();
        std::unique_ptr<ast::Expression> parse_parenthesis();
        std::unique_ptr<ast::Expression> parse_identifier();
        std::unique_ptr<ast::CallExpression> parse_call(const lexer::Token* identifier_token);
        std::unique_ptr<ast::MathematicalNegationExpression> parse_negative();
        std::unique_ptr<ast::CastExpression> parse_cast();

        // Statements
        std::unique_ptr<ast::AssignmentStatement> parse_assignment(const lexer::Token* identifier_token);
        std::unique_ptr<ast::IfStatement> parse_if();
        std::unique_ptr<ast::ForStatement> parse_for();
        std::unique_ptr<ast::ForStatement> create_for_statement(const std::string& variable_identifier, const lexer::Token* variable_data_type_token, std::unique_ptr<ast::Expression> start_value, std::unique_ptr<ast::Expression> end_value, std::unique_ptr<ast::Expression> step_value, const diagnostics::SourceLocation& for_source_location);
        void recover_for_definition_and_parse_body(const diagnostics::SourceLocation& source_location);
        std::unique_ptr<ast::ReturnStatement> parse_return();
        std::unique_ptr<ast::VariableDefinitionStatement> parse_variable_definition();

        template <lexer::TokenType... T>
        struct SynchronizationSet {};

        template <lexer::TokenType... Sync, lexer::TokenType... Consume>
        void recover(SynchronizationSet<Sync...> synchronisation_tokens, SynchronizationSet<Consume...> consume_tokens) {
            while (!synchronisation_tokens_contains<Sync...>(current_token->type) && current_token->type != lexer::TokenType::EndOfFile) {
                next_token(false);
            }
            if (synchronisation_tokens_contains<Consume...>(current_token->type)) {
                next_token(true);
            }
        }

        template <lexer::TokenType... T>
        bool synchronisation_tokens_contains(lexer::TokenType token_type) {
            return ((token_type == T) || ...);
        }

        template <lexer::TokenType... T>
        std::optional<std::vector<std::unique_ptr<ast::ASTNode>>> parse_body(const std::string& diagnostic_message_on_end, const diagnostics::SourceLocation& source_location) {
            std::vector<std::unique_ptr<ast::ASTNode>> body;
            while (((current_token->type != T) && ...)) {
                if (current_token->type == lexer::TokenType::EndOfFile) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, diagnostic_message_on_end, source_location);
                    return std::nullopt;
                }

                std::unique_ptr<ast::ASTNode> statement = parse_statement();
                if (statement) {
                    body.push_back(std::move(statement));
                }
            }
            return body;
        }
    };

}
