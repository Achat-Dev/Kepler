// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/tokenizer.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/source_location.hpp"
#include "io/file.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include "utils/string_utils.hpp"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kepler {

    std::unordered_map<StringId, Token> Tokenizer::keyword_map;

    Tokenizer::Tokenizer(const File* file, DiagnosticSink& diagnostic_sink)
        : file(file), diagnostic_sink(diagnostic_sink) {
        KPL_ASSERT_NOT_NULLPTR(file);
        if (keyword_map.empty()) {
            register_keyword("module", TokenKind::Module);
            register_keyword("import", TokenKind::Import);
            register_keyword("export", TokenKind::Export);
            register_keyword("extern", TokenKind::Extern);
            register_keyword("struct", TokenKind::Struct);
            register_keyword("return", TokenKind::Return);
            register_keyword("end", TokenKind::End);
            register_keyword("if", TokenKind::If);
            register_keyword("else", TokenKind::Else);
            register_keyword("elseif", TokenKind::Elseif);
            register_keyword("for", TokenKind::For);
            register_keyword("true", TokenKind::Literal, true);
            register_keyword("false", TokenKind::Literal, false);
        }
    }

    std::vector<Token> Tokenizer::tokenize() {
        if (file->content.empty()) {
            return {{
                .token_kind = TokenKind::EndOfFile,
                .source_location = {file->id, 0, 0},
            }};
        }

        current_char = file->content[0]; // Read first char manually instead of next_char() because that would read file->content[1]
        std::vector<Token> tokens;
        while (position <= file->content.size()) {
            const Token token = read_next_token();
            tokens.push_back(token);
            if (token.token_kind == TokenKind::EndOfFile) {
                break;
            }
        }

        KPL_ASSERT_THAT(!tokens.empty(), "Tokenizer must create a token stream with at least an EOF token");
        KPL_ASSERT_THAT(tokens.back().token_kind == TokenKind::EndOfFile);
        return tokens;
    }

    int Tokenizer::peek_next_char(uint32_t lookahead) const {
        KPL_ASSERT_THAT(lookahead > 0);
        if (position + lookahead < file->content.size()) {
            return file->content[position + lookahead];
        } else {
            return EOF;
        }
    }

    void Tokenizer::next_char() {
        if (position < file->content.size() - 1) {
            position++;
            current_char = file->content[position];
        } else {
            position = file->content.size();
            current_char = EOF;
        }
    }

    Token Tokenizer::read_next_token() {
        if (current_char == EOF) {
            return {
                .token_kind = TokenKind::EndOfFile,
                .source_location = {file->id, static_cast<uint32_t>(file->content.size()), 1},
            };
        }

        while (isspace(current_char)) {
            if (current_char == '\n') {
                next_char();
                return {
                    .token_kind = TokenKind::Newline,
                    .source_location = {file->id, position, 1},
                };
            }
            next_char();
        }

        if (isalpha(current_char) || current_char == '_') {
            return read_identifier();
        }
        if (isdigit(current_char)) {
            return read_numeric_literal();
        }

        switch (current_char) {
            case '#':
                read_comment();
                return read_next_token();
            case '(':
                next_char();
                return {
                    .token_kind = TokenKind::BracketOpen,
                    .source_location = {file->id, position - 1, 1},
                };
            case ')':
                next_char();
                return {
                    .token_kind = TokenKind::BracketClose,
                    .source_location = {file->id, position - 1, 1},
                };
            case '{':
                next_char();
                return {
                    .token_kind = TokenKind::CurlyBracketOpen,
                    .source_location = {file->id, position - 1, 1},
                };
            case '}':
                next_char();
                return {
                    .token_kind = TokenKind::CurlyBracketClose,
                    .source_location = {file->id, position - 1, 1},
                };
            case '.':
                if (peek_next_char() == '.') {
                    if (peek_next_char(2) == '.') {
                        next_char();
                        next_char();
                        next_char();
                        return {
                            .token_kind = TokenKind::Variadic,
                            .source_location = {file->id, position - 3, 3},
                        };
                    }
                } else {
                    next_char();
                    return {
                        .token_kind = TokenKind::Dot,
                        .source_location = {file->id, position - 1, 1},
                    };
                }
            case ',':
                next_char();
                return {
                    .token_kind = TokenKind::Comma,
                    .source_location = {file->id, position - 1, 1},
                };
            case ':':
                next_char();
                if (current_char == ':') {
                    next_char();
                    return {
                        .token_kind = TokenKind::DoubleColon,
                        .source_location = {file->id, position - 2, 2},
                    };
                } else {
                    return {
                        .token_kind = TokenKind::Colon,
                        .source_location = {file->id, position - 1, 1},
                    };
                }
            case '=':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 2, 2},
                        .data = OperatorKind::Equals,
                    };
                } else {
                    return {
                        .token_kind = TokenKind::Assignment,
                        .source_location = {file->id, position - 1, 1},
                    };
                }
            case '+':
                next_char();
                return {
                    .token_kind = TokenKind::Operator,
                    .source_location = {file->id, position - 1, 1},
                    .data = OperatorKind::Plus,
                };
            case '-':
                next_char();
                return {
                    .token_kind = TokenKind::Operator,
                    .source_location = {file->id, position - 1, 1},
                    .data = OperatorKind::Minus,
                };
            case '*':
                next_char();
                return {
                    .token_kind = TokenKind::Operator,
                    .source_location = {file->id, position - 1, 1},
                    .data = OperatorKind::Multiplication,
                };
            case '/':
                next_char();
                return {
                    .token_kind = TokenKind::Operator,
                    .source_location = {file->id, position - 1, 1},
                    .data = OperatorKind::Division,
                };
            case '<':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 2, 2},
                        .data = OperatorKind::LessEquals,
                    };
                } else {
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 1, 1},
                        .data = OperatorKind::LessThan,
                    };
                }
            case '>':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 2, 2},
                        .data = OperatorKind::GreaterEquals,
                    };
                } else {
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 1, 1},
                        .data = OperatorKind::GreaterThan,
                    };
                }
            case '!':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return {
                        .token_kind = TokenKind::Operator,
                        .source_location = {file->id, position - 2, 2},
                        .data = OperatorKind::NotEquals,
                    };
                } else {
                    diagnostic_sink.report(DiagnosticCode::Unsupported, "Logical negation with '!' is not supported yet", {file->id, position - 1, 1});
                    next_char();
                    return read_next_token();
                }
            case '"': return read_string_literal();
        }

        std::string unknown_char;
        const uint32_t start_position = position;
        if (is_utf8_character(static_cast<unsigned char>(current_char))) {
            while (is_utf8_character(static_cast<unsigned char>(current_char))) {
                unknown_char += current_char;
                next_char(); // eat unknown utf8 code point
            }
        } else {
            unknown_char += current_char;
            next_char(); // eat unknown char
        }
        diagnostic_sink.report(DiagnosticCode::UnknownCharacter,
            std::format("Unknown character '{}'", unknown_char),
            {
                file->id,
                start_position,
                position - start_position,
            });
        return read_next_token();
    }

    Token Tokenizer::read_identifier() {
        int is_current_char_alpha = isalpha(current_char) || current_char == '_';
        KPL_ASSERT_THAT(is_current_char_alpha, "Tokenizing identifier requires character, received '{}'", static_cast<char>(current_char));
        const uint32_t identifier_start_position = position;
        do {
            next_char();
        } while (isalnum(current_char) || current_char == '_');

        const uint32_t identifier_length = position - identifier_start_position;
        KPL_ASSERT_THAT(identifier_length > 0);
        KPL_ASSERT_THAT(file->content.size() >= identifier_start_position + identifier_length);
        const StringId identifier_id = StringPool::get().store(file->content.substr(identifier_start_position, identifier_length));

        if (keyword_map.contains(identifier_id)) {
            Token token = keyword_map[identifier_id];
            token.source_location.file_id = file->id;
            token.source_location.position = identifier_start_position;
            return token;
        }

        return {
            .token_kind = TokenKind::Identifier,
            .source_location = {file->id, identifier_start_position, identifier_length},
            .data = identifier_id,
        };
    }

    Token Tokenizer::read_string_literal() {
        KPL_ASSERT_THAT(current_char == '"', "Tokenizing a string literal requires current_char to be '\"', received '{}'", current_char);
        std::string literal = "";
        next_char(); // eat '"'

        while (current_char != '"') {
            if (current_char == '\\') {
                next_char(); // eat the character to escape

                switch (current_char) {
                    case 'n': literal += '\n'; break;
                    case 't': literal += '\t'; break;
                    case '\\': literal += '\\'; break;
                    case '"': literal += '"'; break;
                    default:
                        diagnostic_sink.report(DiagnosticCode::UnknownEscapeSequence,
                            std::format("Unknown escape sequence '\\{}' in string", current_char),
                            {file->id, position - 1, 2});
                        break;
                }
            } else {
                literal += current_char;
            }
            next_char();
        }

        next_char(); // eat closing '"'

        const uint32_t literal_length = literal.size();
        const StringId literal_id = StringPool::get().store(std::move(literal));
        return {
            .token_kind = TokenKind::Literal,
            .source_location = {file->id, position - literal_length - 2, literal_length + 2}, // -2 and +2 for opening and closing "
            .data = literal_id,
        };
    }

    Token Tokenizer::read_numeric_literal() {
        int is_current_char_digit = isdigit(current_char);
        KPL_ASSERT_THAT(is_current_char_digit, "Tokenizing numeric literal requires digit, received '{}'", current_char);
        const uint32_t literal_start_position = position;
        bool is_float = false;
        do {
            next_char();
            if (current_char == '.') {
                is_float = true;
            }
        } while (isdigit(current_char) || current_char == '.');

        const uint32_t literal_length = position - literal_start_position;
        KPL_ASSERT_THAT(literal_length > 0);
        KPL_ASSERT_THAT(file->content.size() >= literal_start_position + literal_length);
        const std::string literal = file->content.substr(literal_start_position, literal_length);

        if (is_float) {
            return {
                .token_kind = TokenKind::Literal,
                .source_location = {file->id, literal_start_position, literal_length},
                .data = std::stod(literal.data()),
            };
        } else {
            return {
                .token_kind = TokenKind::Literal,
                .source_location = {file->id, literal_start_position, literal_length},
                .data = IntegerLiteralTokenData{.literal_id = StringPool::get().store(literal)},
            };
        }
    }

    void Tokenizer::read_comment() {
        KPL_ASSERT_THAT(current_char == '#', "Reading comment requires '#', received '{}'", current_char);
        next_char(); // eat '#'

        // Two # after each other -> multiline comment
        if (current_char == '#') {
            const uint32_t comment_start_position = position - 1;
            while (!(current_char == '#' && peek_next_char() == '#')) {
                if (peek_next_char() == EOF) {
                    diagnostic_sink.report(DiagnosticCode::MultilineCommentNotClosed,
                        "Multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.",
                        {file->id, comment_start_position, 2});
                    return;
                }

                next_char();
            }

            next_char(); // eat first closing '#'
            next_char(); // eat second closing '#'
        }
        // Single line comment
        else {
            while (current_char != '\n') {
                if (peek_next_char() == EOF) {
                    return;
                }

                next_char();
            }
        }
    }

    void Tokenizer::register_keyword(const std::string& keyword, TokenKind token_kind, TokenData token_data) {
        const StringId keyword_id = StringPool::get().store(keyword);
        keyword_map.emplace(
            keyword_id,
            Token{
                .token_kind = token_kind,
                .source_location = {file->id, 0, static_cast<uint32_t>(keyword.size())},
                .data = token_data,
            });
    }
}
