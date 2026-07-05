// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "log.hpp"
#include <format>
#include <string>

namespace kepler::diagnostics {

    enum class ErrorCode {
        UsageNoInputFile = 101,
        UsageNoOutputFile = 102,
        UsageTooManyInputFiles = 103,
        UsageTooManyOutputFiles = 104,
        UsageFrameworkException = 105,

        IOFileNotFound = 201,
        IOFileIsADirectory = 202,
        IONotARegularFile = 203,
        IOFailedToCreateFileStream = 204,

        LexerUnknownCharacter = 301,
        LexerUnknownEscapeSequence = 302,

        ParserUnexpectedToken = 401,
        ParserInvalidCast = 402,
        ParserInvalidReturnExpression = 403,
        ParserInvalidVariableType = 404,
        ParserInvalidLoopVariableType = 405,
        ParserUnsupportedMathematicalNegation = 406,
        ParserUndefinedSymbol = 407,
        ParserMissingEndKeyword = 408,
        ParserUsingStatementAsExpression = 409,

        SymbolTableRedefineSymbol = 501,

        Unsupported = 999,
    };

}

template <>
struct std::formatter<kepler::diagnostics::ErrorCode> : std::formatter<std::string> {
    auto format(const kepler::diagnostics::ErrorCode& error_code, std::format_context& ctx) const {
        switch (error_code) {
            case kepler::diagnostics::ErrorCode::UsageNoInputFile:
            case kepler::diagnostics::ErrorCode::UsageNoOutputFile:
            case kepler::diagnostics::ErrorCode::UsageTooManyInputFiles:
            case kepler::diagnostics::ErrorCode::UsageTooManyOutputFiles:
                return std::formatter<std::string>::format(std::format("{}{}[ Usage error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::IOFileNotFound:
            case kepler::diagnostics::ErrorCode::IOFileIsADirectory:
            case kepler::diagnostics::ErrorCode::IONotARegularFile:
            case kepler::diagnostics::ErrorCode::IOFailedToCreateFileStream:
                return std::formatter<std::string>::format(std::format("{}{}[ I/O error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::LexerUnknownCharacter:
                return std::formatter<std::string>::format(std::format("{}{}[ Lexing error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
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
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::SymbolTableRedefineSymbol:
                return std::formatter<std::string>::format(std::format("{}{}[ Symbol table error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);

            case kepler::diagnostics::ErrorCode::Unsupported:
                return std::formatter<std::string>::format(std::format("{}{}[ Unpaid developer error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_magenta,
                                                               kepler::log::styling::reset),
                    ctx);
            default:
                kepler::log::warning("Missing format implementation for error code '{}'", static_cast<int>(error_code));
                return std::formatter<std::string>::format(std::format(""), ctx);
        }
    }
};
