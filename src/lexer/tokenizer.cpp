// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/tokenizer.hpp"
#include "error_code.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler::lexer {

    std::unordered_map<std::string, Token> Tokenizer::keyword_map = {
        {"extern", Token(TokenType::Extern)},
        {"return", Token(TokenType::Return)},
        {"end", Token(TokenType::End)},
        {"if", Token(TokenType::If)},
        {"else", Token(TokenType::Else)},
        {"elseif", Token(TokenType::Elseif)},
        {"for", Token(TokenType::For)},
        {"true", Token(TokenType::True)},
        {"false", Token(TokenType::False)},
        {"void", Token(TokenType::DataType, type_system::DataTypeKind::Void)},
        {"tmap", Token(TokenType::DataType, type_system::DataTypeKind::TMap)},
        {"bool", Token(TokenType::DataType, type_system::DataTypeKind::Bool)},
        {"char", Token(TokenType::DataType, type_system::DataTypeKind::Char)},
        {"string", Token(TokenType::DataType, type_system::DataTypeKind::String)},
        {"i8", Token(TokenType::DataType, type_system::DataTypeKind::Int8)},
        {"i16", Token(TokenType::DataType, type_system::DataTypeKind::Int16)},
        {"i32", Token(TokenType::DataType, type_system::DataTypeKind::Int32)},
        {"i64", Token(TokenType::DataType, type_system::DataTypeKind::Int64)},
        {"f32", Token(TokenType::DataType, type_system::DataTypeKind::Float32)},
        {"f64", Token(TokenType::DataType, type_system::DataTypeKind::Float64)},
    };

    std::expected<std::vector<Token>, ErrorCode> Tokenizer::tokenize() {
        log_verbose("Tokenizing file '", file_path, "'");

        // Check if the file at the given path can be used
        if (!std::filesystem::exists(file_path)) {
            log(log_type::IO_ERROR, "[ Reading '", file_path, "' ]: File doesn't exist");
            return std::unexpected(ErrorCode::IOFileNotFound);
        }
        if (std::filesystem::is_directory(file_path)) {
            log(log_type::IO_ERROR, "[ Reading '", file_path, "' ]: Path is a directory");
            return std::unexpected(ErrorCode::IOFileIsADirectory);
        }
        if (!std::filesystem::is_regular_file(file_path)) {
            log(log_type::IO_ERROR, "[ Reading '", file_path, "' ]: File is not a regular file");
            return std::unexpected(ErrorCode::IONotARegularFile);
        }

        // Read file contents into string
        std::ifstream file_stream(file_path);
        if (!file_stream) {
            log(log_type::IO_ERROR, "[ Reading '", file_path, "' ]: ¯\\_(ツ)_/¯\n",
                log_type::INDENTED, "Check the file permissions\n",
                log_type::LAST_INDENTED, "Check if the file is currently locked by other programs");
            return std::unexpected(ErrorCode::IOFailedToCreateFileStream);
        }

        file_content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

        // Tokenize file contents
        current_char = file_content[0]; // Read first char manually instead of next_char() because that would read file_content[1]
        std::vector<Token> result;
        while (true) {
            Token token = read_next_token();
            if (token.type == TokenType::Unknown) {
                return std::unexpected(ErrorCode::LexerUnknownCharacter);
            }

            result.push_back(token);
            if (token.type == TokenType::EndOfFile) {
                log_verbose_no_prefix(log_type::LAST_INDENTED, "Created token: ", token);
                log_verbose("Successfully tokenized file '", file_path, "'");
                return result;
            } else {
                log_verbose_no_prefix(log_type::INDENTED, "Created token: ", token);
            }
        }
    }

    char Tokenizer::peek_next_char() const {
        if (position + 1 < file_content.size()) {
            return file_content[position + 1];
        } else {
            return EOF;
        }
    }

    void Tokenizer::next_char() {
        if (position < file_content.size()) {
            position++;
            current_char = file_content[position];
        }
    }

    Token Tokenizer::read_next_token() {
        if (peek_next_char() == EOF) {
            return Token(TokenType::EndOfFile);
        }

        while (isspace(current_char)) {
            next_char();
        }

        if (isalpha(current_char)) {
            return read_identifier();
        }
        if (isdigit(current_char)) {
            return read_numeric_literal();
        }

        switch (current_char) {
            case '#': return read_comment();
            case ',':
                next_char();
                return Token(TokenType::Comma);
            case ':':
                next_char();
                return Token(TokenType::Colon);
            case '(':
                next_char();
                return Token(TokenType::BracketOpen);
            case ')':
                next_char();
                return Token(TokenType::BracketClose);
            case '=':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, OperatorType::Equals);
                } else {
                    return Token(TokenType::Operator, OperatorType::Assignment);
                }
            case '+':
                next_char();
                return Token(TokenType::Operator, OperatorType::Plus);
            case '-':
                next_char();
                return Token(TokenType::Operator, OperatorType::Minus);
            case '*':
                next_char();
                return Token(TokenType::Operator, OperatorType::Multiplication);
            case '/':
                next_char();
                return Token(TokenType::Operator, OperatorType::Division);
            case '<':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, OperatorType::LessEquals);
                } else {
                    return Token(TokenType::Operator, OperatorType::LessThan);
                }
            case '>':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, OperatorType::GreaterEquals);
                } else {
                    return Token(TokenType::Operator, OperatorType::GreaterThan);
                }
            case '!':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, OperatorType::NotEquals);
                } else {
                    log(log_type::UNSUPPORTED, "Logical negation with '!' is not supported yet");
                    current_char = '!';
                    break;
                }
            case '"': return read_string_literal();
        }

        log(log_type::LEXING_ERROR, "Unknown character '", current_char, "' while lexing");
        return Token(TokenType::Unknown);
    }

    Token Tokenizer::read_identifier() {
        size_t identifier_start_position = position;
        next_char();
        while (isalnum(current_char) || current_char == '_') {
            next_char();
        }

        size_t identifier_length = position - identifier_start_position;
        std::string identifier = file_content.substr(identifier_start_position, identifier_length);

        if (keyword_map.contains(identifier)) {
            return keyword_map[identifier];
        }
        Token token(TokenType::Identifier, identifier);
        return token;
    }

    Token Tokenizer::read_string_literal() {
        // String literals can't be string_views because escape characters have to be interpreted
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
                        log(log_type::LEXING_ERROR, "Unknown escape character '\\", current_char, "' in string");
                        return Token(TokenType::Unknown);
                }

                next_char(); // read the next character for the next loop interation
            } else {
                literal += current_char;
                next_char();
            }
        }

        next_char(); // eat closing '"'
        return Token(TokenType::StringLiteral, literal);
    }

    Token Tokenizer::read_numeric_literal() {
        size_t literal_start_position = position;
        bool is_float = false;
        do {
            next_char();
            if (current_char == '.') {
                is_float = true;
            }
        } while (isdigit(current_char) || current_char == '.');

        size_t literal_length = position - literal_start_position;
        std::string literal = file_content.substr(literal_start_position, literal_length);

        if (is_float) {
            double floating_point_literal = std::stod(literal.data());
            return Token(TokenType::FloatingPointLiteral, floating_point_literal);
        } else {
            int64_t integer_literal = std::stoll(literal.data());
            return Token(TokenType::IntegerLiteral, integer_literal);
        }
    }

    Token Tokenizer::read_comment() {
        next_char();

        // Two # after each other -> multiline comment
        if (current_char == '#') {
            log_verbose_no_prefix(log_type::INDENTED, "Reading multiline comment");
            while (!(current_char == '#' && peek_next_char() == '#')) {
                if (peek_next_char() == EOF) {
                    log(log_type::LEXING_WARNING, "Multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.");
                    return Token(TokenType::EndOfFile);
                }

                next_char();
            }

            next_char(); // eat second '#'
            next_char(); // prepare reading of next token
        }
        // Single line comment
        else {
            log_verbose_no_prefix(log_type::INDENTED, "Reading singleline comment");
            while (current_char != '\n') {
                if (peek_next_char() == EOF) {
                    return Token(TokenType::EndOfFile);
                }

                next_char();
            }
        }

        return read_next_token();
    }

}
