// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <format>
#include <string>
#include <variant>

namespace kepler::lexer {

    using TokenData = std::variant<double, // Floating point literals
        int64_t,                           // Integer literals
        std::string,                       // String literals & identifiers
        bool,                              // Boolean literals
        OperatorType,                      // Operators
        type_system::DataTypeKind,         // Data types
        std::monostate>;

    struct Token {
        Token(TokenType type, diagnostics::SourceLocation source_location)
            : type(type), source_location(source_location) {}
        Token(TokenType type, diagnostics::SourceLocation source_location, TokenData data)
            : type(type), source_location(source_location), data(std::move(data)) {}

        TokenType type;
        TokenData data;
        diagnostics::SourceLocation source_location;
    };

}

template <>
struct std::formatter<kepler::lexer::Token> : std::formatter<std::string> {
    auto format(const kepler::lexer::Token& token, std::format_context& ctx) const {
        switch (token.type) {
            case kepler::lexer::TokenType::Newline:
            case kepler::lexer::TokenType::EndOfFile:
            case kepler::lexer::TokenType::BracketOpen:
            case kepler::lexer::TokenType::BracketClose:
            case kepler::lexer::TokenType::Comma:
            case kepler::lexer::TokenType::Colon:
            case kepler::lexer::TokenType::Assignment:
            case kepler::lexer::TokenType::Extern:
            case kepler::lexer::TokenType::Return:
            case kepler::lexer::TokenType::End:
            case kepler::lexer::TokenType::If:
            case kepler::lexer::TokenType::Else:
            case kepler::lexer::TokenType::Elseif:
            case kepler::lexer::TokenType::For:
                return std::formatter<std::string>::format(std::format("{}", token.type), ctx);

            case kepler::lexer::TokenType::Operator:
                return std::formatter<std::string>::format(std::format("{}({})", token.type, std::get<kepler::lexer::OperatorType>(token.data)), ctx);
            case kepler::lexer::TokenType::Identifier: {
                const std::string& identifier = std::get<std::string>(token.data);
                return std::formatter<std::string>::format(std::format("{}({})", token.type, identifier), ctx);
            }
            case kepler::lexer::TokenType::Literal: {
                const std::string format = std::visit([&token](const auto& value) -> std::string {
                    using ValueType = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<ValueType, std::monostate>) {
                        return std::format("{}", token.type);
                    } else {
                        return std::format("{}({})", token.type, value);
                    }
                },
                    token.data);

                return std::formatter<std::string>::format(std::format("{}", format), ctx);
            }
            case kepler::lexer::TokenType::DataType:
                return std::formatter<std::string>::format(std::format("{}({})", token.type, std::get<kepler::type_system::DataTypeKind>(token.data)), ctx);
            default:
                kepler::log::warning("Missing format implementation for token of type '{}'", static_cast<int>(token.type));
                return std::formatter<std::string>::format(std::format("{}", token.type), ctx);
        }
    }
};
