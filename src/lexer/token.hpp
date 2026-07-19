// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <variant>

namespace kepler {

    using TokenData = std::variant<std::monostate,
        double,        // Floating point literals
        int64_t,       // Integer literals
        StringId,      // String literals & identifiers
        bool,          // Boolean literals
        OperatorType,  // Operators
        DataTypeKind>; // Data types

    struct Token {
        TokenType type;
        SourceLocation source_location;
        TokenData data = std::monostate{};
    };

}

template <>
struct std::formatter<kepler::Token> : std::formatter<std::string> {
    auto format(const kepler::Token& token, std::format_context& ctx) const {
        switch (token.type) {
            case kepler::TokenType::Newline:
            case kepler::TokenType::EndOfFile:
            case kepler::TokenType::BracketOpen:
            case kepler::TokenType::BracketClose:
            case kepler::TokenType::Comma:
            case kepler::TokenType::Colon:
            case kepler::TokenType::Assignment:
            case kepler::TokenType::Extern:
            case kepler::TokenType::Return:
            case kepler::TokenType::End:
            case kepler::TokenType::If:
            case kepler::TokenType::Else:
            case kepler::TokenType::Elseif:
            case kepler::TokenType::For:
                return std::formatter<std::string>::format(std::format("{}", token.type), ctx);

            case kepler::TokenType::Operator:
                return std::formatter<std::string>::format(std::format("{}({})", token.type, std::get<kepler::OperatorType>(token.data)), ctx);
            case kepler::TokenType::Identifier: {
                const kepler::StringId identifier_id = std::get<kepler::StringId>(token.data);
                const string_view identifier = kepler::StringPool::get().lookup(identifier_id);
                return std::formatter<std::string>::format(std::format("{}({})", token.type, identifier), ctx);
            }
            case kepler::TokenType::Literal: {
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
            case kepler::TokenType::DataType:
                return std::formatter<std::string>::format(std::format("{}({})", token.type, std::get<kepler::DataTypeKind>(token.data)), ctx);
            default:
                kepler::log::warning("Missing format implementation for token of type '{}'", static_cast<int>(token.type));
                return std::formatter<std::string>::format(std::format("{}", token.type), ctx);
        }
    }
};
