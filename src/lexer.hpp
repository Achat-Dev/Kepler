// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/type_token.hpp"

#include <cstdint>
#include <ostream>
#include <string>

namespace Kepler::Lexer {

    enum class Token {
        // Meta
        EndOfFile,
        Unknown,
        BracketOpen,
        BracketClose,
        Comma,
        Colon,

        // Operators
        Assignment,
        Plus,
        Minus,
        Multiplication,
        Division,
        LessThan,
        GreaterThan,
        Equals,
        NotEquals,
        LessEquals,
        GreaterEquals,

        // Keywords
        Extern,
        Return,
        End,
        If,
        Else,
        Elseif,
        For,
        True,
        False,

        // Primary
        Identifier,
        FloatingPointLiteral,
        IntegerLiteral,
        StringLiteral,
        DataType,

        // Parsing tokens
        // These are necessary for certain parsing operations
        Parsing_Elseif
    };

    std::ostream& operator<<(std::ostream& os, Token token);

    Token read_token();
    std::string get_identifier();
    int64_t get_integer_literal();
    double get_floating_point_literal();
    std::string get_string_literal();
    Type::TypeToken get_type();

}
