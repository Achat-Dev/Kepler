// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/tokenizer.hpp"
#include "diagnostics/diagnostics.hpp"
#include "diagnostics/error_code.hpp"
#include "diagnostics/warning_code.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "semantic_analysis/string_table.hpp"
#include <cstddef>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler::lexer {

    std::unordered_map<std::string, Token> Tokenizer::keyword_map;

    std::expected<std::vector<Token>, diagnostics::ErrorCode> Tokenizer::tokenize() {
        log::verbose("Tokenizing file '{}'", file_path);

        // Check if the file at the given path can be used
        if (!std::filesystem::exists(file_path)) {
            return diagnostics::error(file_path, {}, diagnostics::ErrorCode::IOFileNotFound, "File '{}' doesn't exist", file_path);
        }
        if (std::filesystem::is_directory(file_path)) {
            return diagnostics::error(file_path, {}, diagnostics::ErrorCode::IOFileIsADirectory, "Path '{}' is a directory", file_path);
        }
        if (!std::filesystem::is_regular_file(file_path)) {
            return diagnostics::error(file_path, {}, diagnostics::ErrorCode::IONotARegularFile, "File '{}' is not a regular file", file_path);
        }

        // Read file contents into string
        std::ifstream file_stream(file_path);
        if (!file_stream) {
            return diagnostics::error(file_path, {}, diagnostics::ErrorCode::IOFailedToCreateFileStream, "¯\\_(ツ)_/¯\n{}Check the file permission for {}\n{}Check if the file is currently locked by other programs", log::styling::indented, file_path, log::styling::last_indented);
        }

        file_content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

        // Tokenize file contents
        current_char = file_content[0]; // Read first char manually instead of next_char() because that would read file_content[1]
        std::vector<Token> result;
        while (true) {
            const auto token = read_next_token();
            if (!token) {
                return std::unexpected(token.error());
            }

            result.push_back(*token);
            if (token->type == TokenType::EndOfFile) {
                log::verbose_no_prefix("{}Tokenizing done", log::styling::last_indented);
                return result;
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

    std::expected<Token, diagnostics::ErrorCode> Tokenizer::read_next_token() {
        if (peek_next_char() == EOF) {
            return Token(TokenType::EndOfFile, {});
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
            case '#':
                read_comment();
                return read_next_token();
            case ',':
                next_char();
                return Token(TokenType::Comma, {position - 1, 1});
            case ':':
                next_char();
                return Token(TokenType::Colon, {position - 1, 1});
            case '(':
                next_char();
                return Token(TokenType::BracketOpen, {position - 1, 1});
            case ')':
                next_char();
                return Token(TokenType::BracketClose, {position - 1, 1});
            case '=':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, {position - 2, 2}, OperatorType::Equals);
                } else {
                    return Token(TokenType::Assignment, {position - 1, 1});
                }
            case '+':
                next_char();
                return Token(TokenType::Operator, {position - 1, 1}, OperatorType::Plus);
            case '-':
                next_char();
                return Token(TokenType::Operator, {position - 1, 1}, OperatorType::Minus);
            case '*':
                next_char();
                return Token(TokenType::Operator, {position - 1, 1}, OperatorType::Multiplication);
            case '/':
                next_char();
                return Token(TokenType::Operator, {position - 1, 1}, OperatorType::Division);
            case '<':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, {position - 2, 2}, OperatorType::LessEquals);
                } else {
                    return Token(TokenType::Operator, {position - 1, 1}, OperatorType::LessThan);
                }
            case '>':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, {position - 2, 2}, OperatorType::GreaterEquals);
                } else {
                    return Token(TokenType::Operator, {position - 1, 1}, OperatorType::GreaterThan);
                }
            case '!':
                next_char();
                if (current_char == '=') {
                    next_char();
                    return Token(TokenType::Operator, {position - 2, 2}, OperatorType::NotEquals);
                } else {
                    return diagnostics::error(file_path, {position - 1, 1}, diagnostics::ErrorCode::Unsupported, "Logical negation with '!' is not supported yet");
                }
            case '"': return read_string_literal();
        }

        return diagnostics::error(file_path, {position, 1}, diagnostics::ErrorCode::LexerUnknownCharacter, "Unknown character '{}'", current_char);
    }

    Token Tokenizer::read_identifier() {
        const size_t identifier_start_position = position;
        next_char();
        while (isalnum(current_char) || current_char == '_') {
            next_char();
        }

        const size_t identifier_length = position - identifier_start_position;
        const std::string identifier = file_content.substr(identifier_start_position, identifier_length);

        if (keyword_map.contains(identifier)) {
            Token token = keyword_map.at(identifier);
            token.source_location.position = identifier_start_position;
            return token;
        }

        const semantic_analysis::StringId identifier_id = semantic_analysis::StringTable::get().store_or_lookup(identifier);
        const Token token(TokenType::Identifier, {identifier_start_position, identifier_length}, identifier_id);
        return token;
    }

    std::expected<Token, diagnostics::ErrorCode> Tokenizer::read_string_literal() {
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
                        return diagnostics::error(file_path, {position - 1, 2}, diagnostics::ErrorCode::LexerUnknownEscapeSequence, "Unknown escape character '\\{}' in string", current_char);
                }

                next_char(); // read the next character for the next loop interation
            } else {
                literal += current_char;
                next_char();
            }
        }

        next_char(); // eat closing '"'

        const semantic_analysis::StringId literal_id = semantic_analysis::StringTable::get().store_or_lookup(literal);
        const size_t literal_length = literal.size();
        return Token(TokenType::Literal, {position - literal_length, literal_length}, literal_id);
    }

    Token Tokenizer::read_numeric_literal() {
        const size_t literal_start_position = position;
        bool is_float = false;
        do {
            next_char();
            if (current_char == '.') {
                is_float = true;
            }
        } while (isdigit(current_char) || current_char == '.');

        const size_t literal_length = position - literal_start_position;
        const std::string literal = file_content.substr(literal_start_position, literal_length);

        if (is_float) {
            return Token(TokenType::Literal, {literal_start_position}, std::stod(literal.data()));
        } else {
            return Token(TokenType::Literal, {literal_start_position, literal_length}, std::stoll(literal.data()));
        }
    }

    void Tokenizer::read_comment() {
        next_char();

        // Two # after each other -> multiline comment
        if (current_char == '#') {
            const size_t comment_start_position = position - 1;
            while (!(current_char == '#' && peek_next_char() == '#')) {
                if (peek_next_char() == EOF) {
                    diagnostics::warning(file_path, {comment_start_position, 2}, diagnostics::WarningCode::LexerMultilineCommentNotClosed, "Multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.");
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
        keyword_map.emplace(keyword, Token(token_type, {0, keyword.size()}, token_data));
    }

}
