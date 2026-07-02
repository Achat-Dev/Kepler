// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <ostream>

namespace kepler::lexer {

    enum class TokenType {
        // Meta
        Unknown,
        EndOfFile,
        BracketOpen,
        BracketClose,
        Comma,
        Colon,
        Assignment,

        // Primary
        Identifier,
        DataType,
        Operator,
        Literal,

        // Keywords
        Extern,
        Return,
        End,
        If,
        Else,
        Elseif,
        For,
    };

    std::ostream& operator<<(std::ostream& os, TokenType type);

}
