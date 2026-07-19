// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "string_pool.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace kepler {

    class Tokenizer {
    public:
        Tokenizer(const File& file, DiagnosticSink& diagnostic_sink);
        std::vector<Token> tokenize();

    private:
        char peek_next_char() const;
        void next_char();
        Token read_next_token();
        Token read_identifier();
        Token read_string_literal();
        Token read_numeric_literal();
        void read_comment();
        void register_keyword(const std::string& keyword, TokenType token_type, TokenData token_data = std::monostate{});

        const File& file;
        DiagnosticSink& diagnostic_sink;
        char current_char = ' ';
        uint32_t position = 0;

        static std::unordered_map<StringId, Token> keyword_map;
    };

}
