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
#include "log.hpp"
#include "string_pool.hpp"
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kepler {

    std::unordered_map<StringId, Token> Tokenizer::keyword_map;

    Tokenizer::Tokenizer(const File& file, DiagnosticSink& diagnostic_sink) : file(file), diagnostic_sink(diagnostic_sink) {
        if (keyword_map.empty()) {
            register_keyword("extern", TokenType::Extern);
            register_keyword("return", TokenType::Return);
            register_keyword("end", TokenType::End);
            register_keyword("if", TokenType::If);
            register_keyword("else", TokenType::Else);
            register_keyword("elseif", TokenType::Elseif);
            register_keyword("for", TokenType::For);
            register_keyword("true", TokenType::Literal, true);
            register_keyword("false", TokenType::Literal, false);
            register_keyword("void", TokenType::DataType, DataTypeKind::Void);
            register_keyword("bool", TokenType::DataType, DataTypeKind::Bool);
            register_keyword("string", TokenType::DataType, DataTypeKind::String);
            register_keyword("i8", TokenType::DataType, DataTypeKind::Int8);
            register_keyword("i16", TokenType::DataType, DataTypeKind::Int16);
            register_keyword("i32", TokenType::DataType, DataTypeKind::Int32);
            register_keyword("i64", TokenType::DataType, DataTypeKind::Int64);
            register_keyword("f32", TokenType::DataType, DataTypeKind::Float32);
            register_keyword("f64", TokenType::DataType, DataTypeKind::Float64);
        }
    }

    std::vector<Token> Tokenizer::tokenize() {
        const std::filesystem::path* file_path = File::get_path_by_id(file.id);
        log::verbose("Tokenizing file '{}'", file_path->c_str());

        current_char = file.content[0]; // Read first char manually instead of next_char() because that would read file.content[1]
        std::vector<Token> tokens;
        while (true) {
            const std::optional<Token> token = read_next_token();
            if (!token) {
                continue;
            }

            tokens.push_back(*token);
            if (token->type == TokenType::EndOfFile) {
                log::verbose_no_prefix("{}Tokenizing done", log::styling::last_indented);
                return tokens;
            }
        }
    }

    char Tokenizer::peek_next_char() const {
        if (position + 1 < file.content.size()) {
            return file.content[position + 1];
        } else {
            return EOF;
        }
    }

    void Tokenizer::next_char() {
        if (position < file.content.size()) {
            position++;
            current_char = file.content[position];
        }
    }

    Token Tokenizer::read_next_token() {
        if (peek_next_char() == EOF) {
            return Token{
                .type = TokenType::EndOfFile,
                .source_location = {FileId::invalid(), 0, 0},
            };
        }

        while (isspace(current_char)) {
            if (current_char == '\n') {
                next_char();
                return Token{
                    .type = TokenType::Newline,
                    .source_location = {file.id, position, 1},
                };
            }
            next_char();
        }

        if (isalpha(current_char)) {
            return read_identifier();
        }
        if (isdigit(current_char)) {
            return read_numeric_literal();
        }

        switch (current_char) {
            case '#':
                read_comment();
                return read_next_token();
            case ',':
                next_char();
                return Token{
                    .type = TokenType::Comma,
                    .source_location = {file.id, position - 1, 1},
                };
            case ':':
                next_char();
                return Token{
                    .type = TokenType::Colon,
                    .source_location = {file.id, position - 1, 1},
                };
            case '(':
                next_char();
                return Token{
                    .type = TokenType::BracketOpen,
                    .source_location = {file.id, position - 1, 1},
                };
            case ')':
                next_char();
                return Token{
                    .type = TokenType::BracketClose,
                    .source_location = {file.id, position - 1, 1},
                };
            case '=':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 2, 2},
                        .data = OperatorType::Equals,
                    };
                } else {
                    return Token{
                        .type = TokenType::Assignment,
                        .source_location = {file.id, position - 1, 1},
                    };
                }
            case '+':
                next_char();
                return Token{
                    .type = TokenType::Operator,
                    .source_location = {file.id, position - 1, 1},
                    .data = OperatorType::Plus,
                };
            case '-':
                next_char();
                return Token{
                    .type = TokenType::Operator,
                    .source_location = {file.id, position - 1, 1},
                    .data = OperatorType::Minus,
                };
            case '*':
                next_char();
                return Token{
                    .type = TokenType::Operator,
                    .source_location = {file.id, position - 1, 1},
                    .data = OperatorType::Multiplication,
                };
            case '/':
                next_char();
                return Token{
                    .type = TokenType::Operator,
                    .source_location = {file.id, position - 1, 1},
                    .data = OperatorType::Division,
                };
            case '<':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 2, 2},
                        .data = OperatorType::LessEquals,
                    };
                } else {
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 1, 1},
                        .data = OperatorType::LessThan,
                    };
                }
            case '>':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 2, 2},
                        .data = OperatorType::GreaterEquals,
                    };
                } else {
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 1, 1},
                        .data = OperatorType::GreaterThan,
                    };
                }
            case '!':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token{
                        .type = TokenType::Operator,
                        .source_location = {file.id, position - 2, 2},
                        .data = OperatorType::NotEquals,
                    };
                } else {
                    diagnostic_sink.report(DiagnosticCode::Unsupported, "Logical negation with '!' is not supported yet", {file.id, position - 1, 1});
                    next_char();
                    return read_next_token();
                }
            case '"': return read_string_literal();
        }

        diagnostic_sink.report(DiagnosticCode::UnknownCharacter, std::format("Unknown character '{}'", current_char), {file.id, position, 1});
        next_char();
        return read_next_token();
    }

    Token Tokenizer::read_identifier() {
        const uint32_t identifier_start_position = position;
        next_char();
        while (isalnum(current_char) || current_char == '_') {
            next_char();
        }

        const uint32_t identifier_length = position - identifier_start_position;
        const StringId identifier_id = StringPool::get().store(file.content.substr(identifier_start_position, identifier_length));

        if (keyword_map.contains(identifier_id)) {
            Token token = keyword_map[identifier_id];
            token.source_location.position = identifier_start_position;
            return token;
        }

        return Token{
            .type = TokenType::Identifier,
            .source_location = {file.id, identifier_start_position, identifier_length},
            .data = identifier_id,
        };
    }

    Token Tokenizer::read_string_literal() {
        std::string literal = "";
        next_char();

        while (current_char != '"') {
            if (current_char == '\\') {
                next_char(); // read the character to escape

                switch (current_char) {
                    case 'n': literal += '\n'; break;
                    case 't': literal += '\t'; break;
                    case '\\': literal += '\\'; break;
                    case '"': literal += '"'; break;
                    default:
                        diagnostic_sink.report(DiagnosticCode::UnknownEscapeSequence,
                            std::format("Unknown escape sequence '\\{}' in string", current_char),
                            {file.id, position - 1, 2});
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
        return Token{
            .type = TokenType::Literal,
            .source_location = {file.id, position - literal_length - 1, literal_length + 2}, // -1 for opening " and +2 for opening and closing "
            .data = literal_id,
        };
    }

    Token Tokenizer::read_numeric_literal() {
        const uint32_t literal_start_position = position;
        bool is_float = false;
        do {
            next_char();
            if (current_char == '.') {
                is_float = true;
            }
        } while (isdigit(current_char) || current_char == '.');

        const uint32_t literal_length = position - literal_start_position;
        const std::string literal = file.content.substr(literal_start_position, literal_length);

        if (is_float) {
            return Token{
                .type = TokenType::Literal,
                .source_location = {file.id, literal_start_position, literal_length},
                .data = std::stod(literal.data()),
            };
        } else {
            return Token{
                .type = TokenType::Literal,
                .source_location = {file.id, literal_start_position, literal_length},
                .data = std::stoll(literal.data()),
            };
        }
    }

    void Tokenizer::read_comment() {
        next_char();

        // Two # after each other -> multiline comment
        if (current_char == '#') {
            const uint32_t comment_start_position = position - 1;
            while (!(current_char == '#' && peek_next_char() == '#')) {
                if (peek_next_char() == EOF) {
                    diagnostic_sink.report(DiagnosticCode::MultilineCommentNotClosed,
                        "Multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.",
                        {file.id, comment_start_position, 2});
                    return;
                }

                next_char();
            }

            next_char(); // eat second '#'
            next_char(); // prepare reading of next token
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

    void Tokenizer::register_keyword(const std::string& keyword, TokenType token_type, TokenData token_data) {
        const StringId keyword_id = StringPool::get().store(keyword);
        keyword_map.emplace(
            keyword_id,
            Token{
                .type = token_type,
                .source_location = {file.id, 0, static_cast<uint32_t>(keyword.size())},
                .data = token_data,
            });
    }
}
