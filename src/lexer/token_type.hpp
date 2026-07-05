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

namespace kepler::lexer {

    enum class TokenType {
        // Meta
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

}

template <>
struct std::formatter<kepler::lexer::TokenType> : std::formatter<std::string> {
    auto format(const kepler::lexer::TokenType& token_type, std::format_context& ctx) const {
        switch (token_type) {
            case kepler::lexer::TokenType::EndOfFile:
                return std::formatter<std::string>::format("EOF", ctx);
            case kepler::lexer::TokenType::BracketOpen:
                return std::formatter<std::string>::format("(", ctx);
            case kepler::lexer::TokenType::BracketClose:
                return std::formatter<std::string>::format(")", ctx);
            case kepler::lexer::TokenType::Comma:
                return std::formatter<std::string>::format(",", ctx);
            case kepler::lexer::TokenType::Colon:
                return std::formatter<std::string>::format(":", ctx);
            case kepler::lexer::TokenType::Assignment:
                return std::formatter<std::string>::format("=", ctx);
            case kepler::lexer::TokenType::Extern:
                return std::formatter<std::string>::format("extern", ctx);
            case kepler::lexer::TokenType::Return:
                return std::formatter<std::string>::format("return", ctx);
            case kepler::lexer::TokenType::End:
                return std::formatter<std::string>::format("end", ctx);
            case kepler::lexer::TokenType::If:
                return std::formatter<std::string>::format("if", ctx);
            case kepler::lexer::TokenType::Else:
                return std::formatter<std::string>::format("else", ctx);
            case kepler::lexer::TokenType::Elseif:
                return std::formatter<std::string>::format("elseif", ctx);
            case kepler::lexer::TokenType::For:
                return std::formatter<std::string>::format("for", ctx);
            case kepler::lexer::TokenType::Literal:
                return std::formatter<std::string>::format("literal", ctx);
            case kepler::lexer::TokenType::Operator:
                return std::formatter<std::string>::format("operator", ctx);
            case kepler::lexer::TokenType::Identifier:
                return std::formatter<std::string>::format("identifier", ctx);
            case kepler::lexer::TokenType::DataType:
                return std::formatter<std::string>::format("datatype", ctx);
            default:
                kepler::log::warning("Missing format implementation for token type '{}'", static_cast<int>(token_type));
                return std::formatter<std::string>::format(std::format("{}", static_cast<int>(token_type)), ctx);
        }
    }
};
