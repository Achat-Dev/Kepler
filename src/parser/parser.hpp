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

        template <typename... SynchronisationTokens>
        void recover(bool eat_synchronisation_token, SynchronisationTokens... synchronisation_tokens) {
            while (((current_token->type != synchronisation_tokens) && ...) && current_token->type != lexer::TokenType::EndOfFile) {
                next_token(false);
            }
            if (eat_synchronisation_token) {
                next_token(false);
            }
        }

        // TODO: Split in multiple smaller parsers

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
        std::shared_ptr<ast::ForStatement> create_for_statement(semantic_analysis::StringId variable_identifier_id, type_system::DataTypeKind variable_data_type, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value);
        std::vector<std::shared_ptr<ast::ASTNode>> parse_for_body();
        std::shared_ptr<ast::ReturnStatement> parse_return();
        std::shared_ptr<ast::VariableDefinitionStatement> parse_variable_definition();
    };

}
