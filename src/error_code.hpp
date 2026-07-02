// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

namespace kepler {

    enum class ErrorCode {
        IOFileNotFound = 101,
        IOFileIsADirectory = 102,
        IONotARegularFile = 103,
        IOFailedToCreateFileStream = 104,

        LexerUnknownCharacter = 201,

        ParserUnexpectedToken = 301,
        ParserInvalidCast = 302,
        ParserInvalidReturnExpression = 303,
        ParserInvalidVariableType = 304,
        ParserInvalidLoopVariableType = 305,
        ParserUnsupportedMathematicalNegation = 306,
        ParserUndefinedSymbol = 308,
        ParserMissingEndKeyword = 309,
        ParserUsingStatementAsExpression = 310,

        SymbolTableRedefineSymbol = 401,

        Unsupported = 999,
    };

}
