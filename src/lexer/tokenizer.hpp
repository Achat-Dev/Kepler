// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "error_code.hpp"
#include "lexer/token.hpp"
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kepler::Lexer {

    class Tokenizer {
    public:
        Tokenizer(const std::string& file_path) : file_path(file_path) {}
        std::expected<std::vector<Token>, ErrorCode> tokenize();

    private:
        const std::string file_path;
        std::string source;
        char last_char = ' ';
        size_t position = 0;

        char peek_next_char() const;
        char read_next_char();
        Token read_next_token();
        Token read_identifier();
        Token read_string_literal();
        Token read_numeric_literal();
        Token read_comment();

        static std::unordered_map<std::string, Token> identifier_map;
    };

}
