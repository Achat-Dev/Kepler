// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ansi_escape_codes.hpp"
#include <format>
#include <string>

namespace kepler::diagnostics {

    enum class ErrorCode {
        IOFileNotFound = 101,
        IOFileIsADirectory = 102,
        IONotARegularFile = 103,
        IOFailedToCreateFileStream = 104,

        LexerUnknownCharacter = 201,
        LexerUnknownEscapeSequence = 201,

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

template <>
struct std::formatter<kepler::diagnostics::ErrorCode> : std::formatter<std::string> {
    auto format(const kepler::diagnostics::ErrorCode& error_code, std::format_context& ctx) const {
        switch (error_code) {
            case kepler::diagnostics::ErrorCode::IOFileNotFound:
            case kepler::diagnostics::ErrorCode::IOFileIsADirectory:
            case kepler::diagnostics::ErrorCode::IONotARegularFile:
            case kepler::diagnostics::ErrorCode::IOFailedToCreateFileStream:
                return std::formatter<std::string>::format(std::format("{}{}[ I/O error ]{}: ",
                                                               kepler::ansi_escape_codes::styles::bold,
                                                               kepler::ansi_escape_codes::colours::bg_red,
                                                               kepler::ansi_escape_codes::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::LexerUnknownCharacter:
                return std::formatter<std::string>::format(std::format("{}{}[ Lexing error ]{}: ",
                                                               kepler::ansi_escape_codes::styles::bold,
                                                               kepler::ansi_escape_codes::colours::bg_red,
                                                               kepler::ansi_escape_codes::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::ParserUnexpectedToken:
            case kepler::diagnostics::ErrorCode::ParserInvalidCast:
            case kepler::diagnostics::ErrorCode::ParserInvalidReturnExpression:
            case kepler::diagnostics::ErrorCode::ParserInvalidVariableType:
            case kepler::diagnostics::ErrorCode::ParserInvalidLoopVariableType:
            case kepler::diagnostics::ErrorCode::ParserUnsupportedMathematicalNegation:
            case kepler::diagnostics::ErrorCode::ParserUndefinedSymbol:
            case kepler::diagnostics::ErrorCode::ParserMissingEndKeyword:
            case kepler::diagnostics::ErrorCode::ParserUsingStatementAsExpression:
                return std::formatter<std::string>::format(std::format("{}{}[ Parsing error ]{}: ",
                                                               kepler::ansi_escape_codes::styles::bold,
                                                               kepler::ansi_escape_codes::colours::bg_red,
                                                               kepler::ansi_escape_codes::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::SymbolTableRedefineSymbol:
                return std::formatter<std::string>::format(std::format("{}{}[ Symbol table error ]{}: ",
                                                               kepler::ansi_escape_codes::styles::bold,
                                                               kepler::ansi_escape_codes::colours::bg_red,
                                                               kepler::ansi_escape_codes::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::Unsupported:
                return std::formatter<std::string>::format(std::format("{}{}[ Unpaid developer error ]{}: ",
                                                               kepler::ansi_escape_codes::styles::bold,
                                                               kepler::ansi_escape_codes::colours::bg_magenta,
                                                               kepler::ansi_escape_codes::reset),
                    ctx);
            default:
                return std::formatter<std::string>::format(std::format(""), ctx);
        }
    }
};
