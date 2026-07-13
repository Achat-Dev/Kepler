// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "ast/ast_node.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "emergency.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include <format>
#include <memory>
#include <vector>

namespace kepler::parser {

    void Parser::next_token(bool skip_newline) {
        if (current_token_index < tokens.size() - 1) {
            current_token_index++;
            current_token = &tokens[current_token_index];

            if (current_token->type == lexer::TokenType::Newline && skip_newline) {
                next_token(skip_newline);
            }
        }
    }

    void Parser::previous_token(bool skip_newline) {
        if (current_token_index > 0) {
            current_token_index--;
            current_token = &tokens[current_token_index];

            if (current_token->type == lexer::TokenType::Newline && skip_newline) {
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

    std::vector<std::shared_ptr<ast::ASTNode>> Parser::parse() {
        log::verbose("Parsing token stream");

        if (tokens.back().type != lexer::TokenType::EndOfFile) {
            emergency_exit("Parser received token stream without EOF token, my tokenizer seems to have fucked up somewhere");
        }

        std::vector<std::shared_ptr<ast::ASTNode>> result;
        while (true) {
            switch (current_token->type) {
                case lexer::TokenType::DataType: {
                    const std::shared_ptr<ast::ASTNode> ast_node = parse_top_level_data_type();
                    if (ast_node) {
                        result.push_back(ast_node);
                    }
                    break;
                }
                case lexer::TokenType::EndOfFile:
                    log::verbose_no_prefix("{} Parsing done", log::styling::last_indented);
                    return result;
                case lexer::TokenType::Extern: {
                    const std::shared_ptr<ast::ASTNode> ast_node = parse_extern();
                    if (ast_node) {
                        result.push_back(ast_node);
                    }
                    break;
                }
                case lexer::TokenType::Newline:
                    next_token(true);
                    break;
                default:
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}' on top level, expected 'extern' or function definition", current_token->type), file_path, current_token->source_location);
                    recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                    break;
            }
        }
    }

}
