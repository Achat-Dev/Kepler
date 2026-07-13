// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/negation_expression.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "semantic_analysis/string_table.hpp"
#include "semantic_analysis/symbol_id.hpp"
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
        Parser(const std::vector<lexer::Token>& tokens, const std::string& file_path, diagnostics::DiagnosticSink& diagnostic_sink)
            : tokens(tokens), file_path(file_path), diagnostic_sink(diagnostic_sink), current_token(&tokens[0]) {}
        std::vector<std::shared_ptr<ast::ASTNode>> parse();

    private:
        const std::vector<lexer::Token>& tokens;
        const std::string& file_path;
        diagnostics::DiagnosticSink& diagnostic_sink;
        const lexer::Token* current_token;
        size_t current_token_index = 0;
        type_system::DataTypeKind current_parsing_function_return_type;

        int get_operator_precedence(lexer::OperatorType operator_type) const;
        void next_token(bool skip_newline);
        void previous_token(bool skip_newline);
        void jump_to_token(size_t index);

        // Top level
        std::shared_ptr<ast::ASTNode> parse_extern();
        std::optional<semantic_analysis::SymbolId> parse_prototype(ast::Prototype::LinkageType linkage_type);
        std::shared_ptr<ast::ASTNode> parse_top_level_data_type();
        std::shared_ptr<ast::Function> parse_function(type_system::DataTypeKind return_type);

        // Main body nodes
        std::shared_ptr<ast::ASTNode> parse_statement();
        std::shared_ptr<ast::Expression> parse_expression();

        // Expressions
        std::shared_ptr<ast::Expression> parse_binary_expression_rhs(std::shared_ptr<ast::Expression> lhs_expression, int expression_precedence);
        std::shared_ptr<ast::Expression> parse_primary();
        std::shared_ptr<ast::Expression> parse_literal();
        std::shared_ptr<ast::Expression> parse_parenthesis();
        std::shared_ptr<ast::Expression> parse_identifier();
        std::shared_ptr<ast::CallExpression> parse_call(semantic_analysis::StringId identifier_id);
        std::shared_ptr<ast::NegationExpression> parse_negative();
        std::shared_ptr<ast::CastExpression> parse_cast();

        // Statements
        std::shared_ptr<ast::AssignmentStatement> parse_assignment(semantic_analysis::StringId identifier_id);
        std::shared_ptr<ast::IfStatement> parse_if();
        std::shared_ptr<ast::ForStatement> parse_for();
        std::shared_ptr<ast::ForStatement> create_for_statement(semantic_analysis::StringId variable_identifier_id, const lexer::Token* variable_data_type_token, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value, const diagnostics::SourceLocation& for_source_location);
        void recover_for_definition_and_parse_body(const diagnostics::SourceLocation& source_location);
        std::shared_ptr<ast::ReturnStatement> parse_return();
        std::shared_ptr<ast::VariableDefinitionStatement> parse_variable_definition();

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
        std::optional<std::vector<std::shared_ptr<ast::ASTNode>>> parse_body(const std::string& diagnostic_message_on_end, const diagnostics::SourceLocation& source_location) {
            std::vector<std::shared_ptr<ast::ASTNode>> body;
            while (((current_token->type != T) && ...)) {
                if (current_token->type == lexer::TokenType::EndOfFile) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, diagnostic_message_on_end, file_path, source_location);
                    return std::nullopt;
                }

                const std::shared_ptr<ast::ASTNode> statement = parse_statement();
                if (statement) {
                    body.push_back(statement);
                }
            }
            return body;
        }
    };

}
