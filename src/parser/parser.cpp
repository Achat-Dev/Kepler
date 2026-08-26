// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "diagnostics/diagnostic.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include <format>
#include <memory>
#include <vector>

namespace kepler {

    void Parser::next_token(bool skip_newline) {
        if (current_token_index < tokens.size() - 1) {
            current_token_index++;
            current_token = &tokens[current_token_index];

            if (current_token->type == TokenType::Newline && skip_newline) {
                next_token(skip_newline);
            }
        }
    }

    void Parser::previous_token(bool skip_newline) {
        if (current_token_index > 0) {
            current_token_index--;
            current_token = &tokens[current_token_index];

            if (current_token->type == TokenType::Newline && skip_newline) {
                previous_token(skip_newline);
            }
        }
    }

    void Parser::jump_to_token(size_t index) {
        KPL_ASSERT_THAT(index <= tokens.size(), "Can't jump to out of range token");
        current_token_index = index;
        current_token = &tokens[current_token_index];
    }

    AbstractSyntaxTree Parser::parse() {
        KPL_ASSERT_THAT(!tokens.empty(), "Token stream must have a size > 0 for parsing");
        KPL_ASSERT_THAT(tokens.back().type == TokenType::EndOfFile,
            "Token stream must end with EOF token for parsing, received stream that ends with '{}' token",
            tokens.back().type);

        AbstractSyntaxTree result;
        while (current_token->type != TokenType::EndOfFile) {
            switch (current_token->type) {
                case TokenType::Type: {
                    std::unique_ptr<ASTNode> ast_node = parse_top_level_type();
                    if (ast_node) {
                        result.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Extern: {
                    std::unique_ptr<ASTNode> ast_node = parse_extern();
                    if (ast_node) {
                        result.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Newline:
                    next_token(true);
                    break;
                default:
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        std::format("Unexpected token '{}' on top level, expected 'extern' or function definition", current_token->type),
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                    break;
            }
        }

        return result;
    }

}
