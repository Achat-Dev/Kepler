// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/error_code.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "type_system/data_type_kind.hpp"
#include <expected>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace kepler::lexer {

    class Tokenizer {
    public:
        Tokenizer(const std::string& file_path) : file_path(file_path) {
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
                register_keyword("void", TokenType::DataType, type_system::DataTypeKind::Void);
                register_keyword("bool", TokenType::DataType, type_system::DataTypeKind::Bool);
                register_keyword("char", TokenType::DataType, type_system::DataTypeKind::Char);
                register_keyword("string", TokenType::DataType, type_system::DataTypeKind::String);
                register_keyword("i8", TokenType::DataType, type_system::DataTypeKind::Int8);
                register_keyword("i16", TokenType::DataType, type_system::DataTypeKind::Int16);
                register_keyword("i32", TokenType::DataType, type_system::DataTypeKind::Int32);
                register_keyword("i64", TokenType::DataType, type_system::DataTypeKind::Int64);
                register_keyword("f32", TokenType::DataType, type_system::DataTypeKind::Float32);
                register_keyword("f64", TokenType::DataType, type_system::DataTypeKind::Float64);
            }
        }
        std::expected<std::vector<Token>, diagnostics::ErrorCode> tokenize();

    private:
        char peek_next_char() const;
        void next_char();
        std::expected<Token, diagnostics::ErrorCode> read_next_token();
        Token read_identifier();
        std::expected<Token, diagnostics::ErrorCode> read_string_literal();
        Token read_numeric_literal();
        void read_comment();
        void register_keyword(const std::string& keyword, TokenType token_type, TokenData token_data = std::monostate{});

        const std::string& file_path;
        std::string file_content;
        char current_char = ' ';
        size_t position = 0;

        static std::unordered_map<std::string, Token> keyword_map;
    };

}
