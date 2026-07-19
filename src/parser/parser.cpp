// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "assert.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
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
        if (index < 0) {
            log::warning("Trying to jump to token at negative index, clamping to 0 (index is '{}')", index);
            index = 0;
        } else if (index >= tokens.size()) {
            log::warning("Trying to jump to token at index out of range, clamping to {} (index is '{}')", tokens.size() - 1, index);
            index = tokens.size() - 1;
        }
        current_token_index = index;
        current_token = &tokens[current_token_index];
    }

    AbstractSyntaxTree Parser::parse() {
        KPL_ASSERT(tokens.back().type == TokenType::EndOfFile, "Parser received token stream without EOF token (last token is '{}'), my tokenizer seems to have fucked up somewhere", tokens.back());
        log::verbose("Parsing token stream");

        AbstractSyntaxTree result;
        while (current_token->type != TokenType::EndOfFile) {
            switch (current_token->type) {
                case TokenType::DataType: {
                    std::unique_ptr<ASTNode> ast_node = parse_top_level_data_type();
                    if (ast_node) {
                        result.nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Extern: {
                    std::unique_ptr<ASTNode> ast_node = parse_extern();
                    if (ast_node) {
                        result.nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Newline:
                    next_token(true);
                    break;
                default:
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}' on top level, expected 'extern' or function definition", current_token->type), current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                    break;
            }
        }

        log::verbose_no_prefix("{} Parsing done", log::styling::last_indented);
        return result;
    }

}
