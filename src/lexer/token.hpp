// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "string_pool.hpp"
#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace kepler {

    enum class TokenType {
        // Meta
        Newline,
        EndOfFile,
        BracketOpen,
        BracketClose,
        Comma,
        Colon,
        Assignment,

        // Primary
        Identifier,
        Type,
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

    using TokenData = std::variant<std::monostate,
        double,        // Floating point literals
        int64_t,       // Integer literals
        StringId,      // String literals, types & identifiers
        bool,          // Boolean literals
        OperatorType>; // Operators

    struct Token {
        TokenType type;
        SourceLocation source_location;
        TokenData data = std::monostate{};
    };

}

template <>
struct std::formatter<kepler::TokenType> : std::formatter<std::string> {
    auto format(const kepler::TokenType& token_type, std::format_context& ctx) const {
        switch (token_type) {
            case kepler::TokenType::Newline:
                return std::formatter<std::string>::format("Newline", ctx);
            case kepler::TokenType::EndOfFile:
                return std::formatter<std::string>::format("EOF", ctx);
            case kepler::TokenType::BracketOpen:
                return std::formatter<std::string>::format("(", ctx);
            case kepler::TokenType::BracketClose:
                return std::formatter<std::string>::format(")", ctx);
            case kepler::TokenType::Comma:
                return std::formatter<std::string>::format(",", ctx);
            case kepler::TokenType::Colon:
                return std::formatter<std::string>::format(":", ctx);
            case kepler::TokenType::Assignment:
                return std::formatter<std::string>::format("=", ctx);
            case kepler::TokenType::Extern:
                return std::formatter<std::string>::format("extern", ctx);
            case kepler::TokenType::Return:
                return std::formatter<std::string>::format("return", ctx);
            case kepler::TokenType::End:
                return std::formatter<std::string>::format("end", ctx);
            case kepler::TokenType::If:
                return std::formatter<std::string>::format("if", ctx);
            case kepler::TokenType::Else:
                return std::formatter<std::string>::format("else", ctx);
            case kepler::TokenType::Elseif:
                return std::formatter<std::string>::format("elseif", ctx);
            case kepler::TokenType::For:
                return std::formatter<std::string>::format("for", ctx);
            case kepler::TokenType::Literal:
                return std::formatter<std::string>::format("literal", ctx);
            case kepler::TokenType::Operator:
                return std::formatter<std::string>::format("operator", ctx);
            case kepler::TokenType::Identifier:
                return std::formatter<std::string>::format("identifier", ctx);
            case kepler::TokenType::Type:
                return std::formatter<std::string>::format("type", ctx);
        }

        assert(false && "Missing format implementation for token type");
        std::unreachable();
    }
};

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
            case kepler::TokenType::Type: {
                const kepler::StringId type_id = std::get<kepler::StringId>(token.data);
                const std::string_view type_name = kepler::StringPool::get().lookup(type_id);
                return std::formatter<std::string>::format(std::format("{}({})", token.type, type_name), ctx);
            }
        }

        assert(false && "Missing format implementation for token of type");
        std::unreachable();
    }
};
