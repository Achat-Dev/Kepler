// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

namespace Kepler::Lexer {

    enum class TokenType {
        // Meta
        Unknown,
        EndOfFile,
        BracketOpen,
        BracketClose,
        Comma,
        Colon,

        // Primary
        Identifier,
        DataType,
        Operator,
        FloatingPointLiteral,
        IntegerLiteral,
        StringLiteral,

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

        // Parsing tokens
        // These are necessary for certain parsing operations
        Parsing_Elseif
    };

}
