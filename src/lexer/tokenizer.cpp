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
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kepler::Lexer {

    std::unordered_map<std::string, Token> Tokenizer::identifier_map = {
        {"extern", Token(TokenType::Extern)},
        {"return", Token(TokenType::Return)},
        {"end", Token(TokenType::End)},
        {"if", Token(TokenType::If)},
        {"else", Token(TokenType::Else)},
        {"elseif", Token(TokenType::Elseif)},
        {"for", Token(TokenType::For)},
        {"true", Token(TokenType::True)},
        {"false", Token(TokenType::False)},
        {"void", Token(TokenType::DataType, TypeSystem::DataTypeKind::Void)},
        {"tmap", Token(TokenType::DataType, TypeSystem::DataTypeKind::TMap)},
        {"bool", Token(TokenType::DataType, TypeSystem::DataTypeKind::Bool)},
        {"char", Token(TokenType::DataType, TypeSystem::DataTypeKind::Char)},
        {"string", Token(TokenType::DataType, TypeSystem::DataTypeKind::String)},
        {"i8", Token(TokenType::DataType, TypeSystem::DataTypeKind::Int8)},
        {"i16", Token(TokenType::DataType, TypeSystem::DataTypeKind::Int16)},
        {"i32", Token(TokenType::DataType, TypeSystem::DataTypeKind::Int32)},
        {"i64", Token(TokenType::DataType, TypeSystem::DataTypeKind::Int64)},
        {"f32", Token(TokenType::DataType, TypeSystem::DataTypeKind::Float32)},
        {"f64", Token(TokenType::DataType, TypeSystem::DataTypeKind::Float64)},
    };

    std::expected<std::vector<Token>, ErrorCode>
    Tokenizer::tokenize() {
        log_verbose("Tokenizing file '", file_path, "'");

        // Check if the file at the given path can be used
        if (!std::filesystem::exists(file_path)) {
            log(LogType::IO_ERROR, "File '", file_path, "' doesn't exist");
            return std::unexpected(ErrorCode::IOFileNotFound);
        }
        if (std::filesystem::is_directory(file_path)) {
            log(LogType::IO_ERROR, "Path '", file_path, "' is a directory");
            return std::unexpected(ErrorCode::IOFileIsADirectory);
        }
        if (!std::filesystem::is_regular_file(file_path)) {
            log(LogType::IO_ERROR, "Path '", file_path, "' is not a regular file");
            return std::unexpected(ErrorCode::IONotARegularFile);
        }

        // Read file contents into string
        std::ifstream file_stream(file_path);
        if (!file_stream) {
            log(LogType::IO_ERROR, "Failed to open file '", file_path, "'\n",
                LogType::INDENTED, "Check the file permissions\n",
                LogType::LAST_INDENTED, "Check if the file is currently opened by other programs");
            return std::unexpected(ErrorCode::IOFailedToCreateFileStream);
        }
        source = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

        // Tokenize file contents
        std::vector<Token> result;
        while (true) {
            Token token = read_next_token();
            if (token.type == TokenType::Unknown) {
                return std::unexpected(ErrorCode::LexerUnknownCharacter);
            }

            result.push_back(token);
            if (token.type == TokenType::EndOfFile) {
                log_verbose_no_prefix(LogType::LAST_INDENTED, "Created token: ", token);
                log_verbose("Successfully tokenized file '", file_path, "'");
                return result;
            } else {
                log_verbose_no_prefix(LogType::INDENTED, "Created token: ", token);
            }
        }
    }

    char Tokenizer::peek_next_char() const {
        if (position + 1 < source.size()) {
            return source[position + 1];
        } else {
            return EOF;
        }
    }

    char Tokenizer::read_next_char() {
        if (position < source.size()) {
            last_char = source[position];
            position++;
            return last_char;
        } else {
            return EOF;
        }
    }

    Token Tokenizer::read_next_token() {
        if (peek_next_char() == EOF) {
            return Token(TokenType::EndOfFile);
        }

        while (isspace(last_char)) {
            last_char = read_next_char();
        }

        if (isalpha(last_char)) {
            return read_identifier();
        }
        if (isdigit(last_char)) {
            return read_numeric_literal();
        }

        switch (last_char) {
            case '#': return read_comment();
            case ',':
                last_char = read_next_char();
                return Token(TokenType::Comma);
            case ':':
                last_char = read_next_char();
                return Token(TokenType::Colon);
            case '(':
                last_char = read_next_char();
                return Token(TokenType::BracketOpen);
            case ')':
                last_char = read_next_char();
                return Token(TokenType::BracketClose);
            case '=':
                last_char = read_next_char();
                if (last_char == '=') {
                    last_char = read_next_char();
                    return Token(TokenType::Operator, OperatorType::Equals);
                } else {
                    return Token(TokenType::Operator, OperatorType::Assignment);
                }
            case '+':
                last_char = read_next_char();
                return Token(TokenType::Operator, OperatorType::Plus);
            case '-':
                last_char = read_next_char();
                return Token(TokenType::Operator, OperatorType::Minus);
            case '*':
                last_char = read_next_char();
                return Token(TokenType::Operator, OperatorType::Multiplication);
            case '/':
                last_char = read_next_char();
                return Token(TokenType::Operator, OperatorType::Division);
            case '<':
                last_char = read_next_char();
                if (last_char == '=') {
                    last_char = read_next_char();
                    return Token(TokenType::Operator, OperatorType::LessEquals);
                } else {
                    return Token(TokenType::Operator, OperatorType::LessThan);
                }
            case '>':
                last_char = read_next_char();
                if (last_char == '=') {
                    last_char = read_next_char();
                    return Token(TokenType::Operator, OperatorType::GreaterEquals);
                } else {
                    return Token(TokenType::Operator, OperatorType::GreaterThan);
                }
            case '!':
                last_char = read_next_char();
                if (last_char == '=') {
                    last_char = read_next_char();
                    return Token(TokenType::Operator, OperatorType::NotEquals);
                } else {
                    log(LogType::UNSUPPORTED, "Logical negation with '!' is not supported yet");
                    last_char = '!';
                    break;
                }
            case '"': return read_string_literal();
        }

        log(LogType::LEXING_ERROR, "Unknown character '", last_char, "' while lexing");
        return Token(TokenType::Unknown);
    }

    Token Tokenizer::read_identifier() {
        std::string identifier(1, last_char);
        last_char = read_next_char();
        while (isalnum(last_char) || last_char == '_') {
            identifier += last_char;
            last_char = read_next_char();
        }

        if (identifier_map.contains(identifier)) {
            return identifier_map[identifier];
        }
        return Token(TokenType::Identifier, identifier);
    }

    Token Tokenizer::read_string_literal() {
        std::string literal = "";
        last_char = read_next_char();

        while (last_char != '"') {
            if (last_char == '\\') {
                last_char = read_next_char(); // read the character to escape

                switch (last_char) {
                    case 'n': literal += '\n'; break;
                    case 't': literal += '\t'; break;
                    case '\\': literal += '\\'; break;
                    case '"': literal += '"'; break;
                    default:
                        log(LogType::LEXING_ERROR, "Unknown escape character '\\", last_char, "' in string");
                        return Token(TokenType::Unknown);
                }

                last_char = read_next_char(); // read the next character for the next loop interation
            } else {
                literal += last_char;
                last_char = read_next_char();
            }
        }

        last_char = read_next_char(); // eat closing '"'
        return Token(TokenType::StringLiteral, literal);
    }

    Token Tokenizer::read_numeric_literal() {
        std::string literal_as_string;
        bool is_float = false;
        do {
            literal_as_string += last_char;
            last_char = read_next_char();
            if (last_char == '.') {
                is_float = true;
            }
        } while (isdigit(last_char) || last_char == '.');

        if (is_float) {
            double floating_point_literal = std::stod(literal_as_string);
            return Token(TokenType::FloatingPointLiteral, floating_point_literal);
        } else {
            int64_t integer_literal = std::stoll(literal_as_string);
            return Token(TokenType::IntegerLiteral, integer_literal);
        }
    }

    Token Tokenizer::read_comment() {
        last_char = read_next_char();

        // Two # after each other -> multiline comment
        if (last_char == '#') {
            log_verbose_no_prefix(LogType::INDENTED, "Reading multiline comment");
            while (!(last_char == '#' && peek_next_char() == '#')) {
                if (peek_next_char() == EOF) {
                    log(LogType::LEXING_WARNING, "Multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.");
                    // log_verbose("\tTokenType: EndOfFile");
                    return Token(TokenType::EndOfFile);
                }

                last_char = read_next_char();
            }

            read_next_char();             // eat second '#'
            last_char = read_next_char(); // save actual next character for reading next token
        }
        // Single line comment
        else {
            log_verbose_no_prefix(LogType::INDENTED, "Reading singleline comment");
            while (last_char != '\n' && last_char != '\r') {
                if (peek_next_char() == EOF) {
                    return Token(TokenType::EndOfFile);
                }

                last_char = read_next_char();
            }
        }

        return read_next_token();
    }

}
