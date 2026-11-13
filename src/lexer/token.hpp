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
#include "lexer/operator_kind.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <string>
#include <variant>

namespace kepler {

    enum class TokenKind {
        // Meta
        Newline,
        EndOfFile,
        BracketOpen,
        BracketClose,
        CurlyBracketOpen,
        CurlyBracketClose,
        Dot,
        Comma,
        Colon,
        DoubleColon,
        Assignment,
        Variadic,

        // Primary
        Identifier,
        Operator,
        Literal,

        // Keywords
        Module,
        Import,
        Export,
        Extern,
        Struct,
        Return,
        End,
        If,
        Else,
        Elseif,
        For,
    };

    struct IntegerLiteralTokenData {
        StringId literal_id;
    };

    using TokenData = std::variant<std::monostate,
        double,                  // Floating point literals
        StringId,                // String literals, types & identifiers
        IntegerLiteralTokenData, // Integer literals
        bool,                    // Boolean literals
        OperatorKind>;           // Operators

    struct Token {
        TokenKind token_kind;
        SourceLocation source_location;
        TokenData data = std::monostate{};
    };

}

template <>
struct std::formatter<kepler::IntegerLiteralTokenData> : std::formatter<std::string> {
    auto format(const kepler::IntegerLiteralTokenData& data, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("{}", kepler::StringPool::get().lookup(data.literal_id)), ctx);
    }
};

template <>
struct std::formatter<kepler::TokenKind> : std::formatter<std::string> {
    auto format(const kepler::TokenKind& token_kind, std::format_context& ctx) const {
        switch (token_kind) {
            case kepler::TokenKind::Newline:
                return std::formatter<std::string>::format("Newline", ctx);
            case kepler::TokenKind::EndOfFile:
                return std::formatter<std::string>::format("EOF", ctx);
            case kepler::TokenKind::BracketOpen:
                return std::formatter<std::string>::format("(", ctx);
            case kepler::TokenKind::BracketClose:
                return std::formatter<std::string>::format(")", ctx);
            case kepler::TokenKind::CurlyBracketOpen:
                return std::formatter<std::string>::format("{", ctx);
            case kepler::TokenKind::CurlyBracketClose:
                return std::formatter<std::string>::format("}", ctx);
            case kepler::TokenKind::Dot:
                return std::formatter<std::string>::format(".", ctx);
            case kepler::TokenKind::Comma:
                return std::formatter<std::string>::format(",", ctx);
            case kepler::TokenKind::Colon:
                return std::formatter<std::string>::format(":", ctx);
            case kepler::TokenKind::DoubleColon:
                return std::formatter<std::string>::format("::", ctx);
            case kepler::TokenKind::Assignment:
                return std::formatter<std::string>::format("=", ctx);
            case kepler::TokenKind::Variadic:
                return std::formatter<std::string>::format("...", ctx);
            case kepler::TokenKind::Module:
                return std::formatter<std::string>::format("module", ctx);
            case kepler::TokenKind::Import:
                return std::formatter<std::string>::format("import", ctx);
            case kepler::TokenKind::Export:
                return std::formatter<std::string>::format("export", ctx);
            case kepler::TokenKind::Extern:
                return std::formatter<std::string>::format("extern", ctx);
            case kepler::TokenKind::Struct:
                return std::formatter<std::string>::format("struct", ctx);
            case kepler::TokenKind::Return:
                return std::formatter<std::string>::format("return", ctx);
            case kepler::TokenKind::End:
                return std::formatter<std::string>::format("end", ctx);
            case kepler::TokenKind::If:
                return std::formatter<std::string>::format("if", ctx);
            case kepler::TokenKind::Else:
                return std::formatter<std::string>::format("else", ctx);
            case kepler::TokenKind::Elseif:
                return std::formatter<std::string>::format("elseif", ctx);
            case kepler::TokenKind::For:
                return std::formatter<std::string>::format("for", ctx);
            case kepler::TokenKind::Literal:
                return std::formatter<std::string>::format("literal", ctx);
            case kepler::TokenKind::Operator:
                return std::formatter<std::string>::format("operator", ctx);
            case kepler::TokenKind::Identifier:
                return std::formatter<std::string>::format("identifier", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for token kind '{}'", static_cast<int>(token_kind));
    }
};

template <>
struct std::formatter<kepler::Token> : std::formatter<std::string> {
    auto format(const kepler::Token& token, std::format_context& ctx) const {
        switch (token.token_kind) {
            case kepler::TokenKind::Newline:
            case kepler::TokenKind::EndOfFile:
            case kepler::TokenKind::BracketOpen:
            case kepler::TokenKind::BracketClose:
            case kepler::TokenKind::CurlyBracketOpen:
            case kepler::TokenKind::CurlyBracketClose:
            case kepler::TokenKind::Dot:
            case kepler::TokenKind::Comma:
            case kepler::TokenKind::Colon:
            case kepler::TokenKind::DoubleColon:
            case kepler::TokenKind::Assignment:
            case kepler::TokenKind::Variadic:
            case kepler::TokenKind::Module:
            case kepler::TokenKind::Import:
            case kepler::TokenKind::Export:
            case kepler::TokenKind::Extern:
            case kepler::TokenKind::Struct:
            case kepler::TokenKind::Return:
            case kepler::TokenKind::End:
            case kepler::TokenKind::If:
            case kepler::TokenKind::Else:
            case kepler::TokenKind::Elseif:
            case kepler::TokenKind::For:
                return std::formatter<std::string>::format(std::format("{}", token.token_kind), ctx);

            case kepler::TokenKind::Operator:
                return std::formatter<std::string>::format(std::format("{}({})", token.token_kind, std::get<kepler::OperatorKind>(token.data)), ctx);
            case kepler::TokenKind::Identifier: {
                const kepler::StringId identifier_id = std::get<kepler::StringId>(token.data);
                const std::string& identifier = kepler::StringPool::get().lookup(identifier_id);
                return std::formatter<std::string>::format(std::format("{}({})", token.token_kind, identifier), ctx);
            }
            case kepler::TokenKind::Literal: {
                const std::string format = std::visit(
                    [&token](const auto& value) -> std::string {
                        using ValueType = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<ValueType, std::monostate>) {
                            return std::format("{}", token.token_kind);
                        } else if constexpr (std::is_same_v<ValueType, kepler::IntegerLiteralTokenData>) {
                            const std::string& literal = kepler::StringPool::get().lookup(value.literal_id);
                            return std::format("{}({})", token.token_kind, literal);
                        } else if constexpr (std::is_same_v<ValueType, kepler::StringId>) {
                            const std::string& literal = kepler::StringPool::get().lookup(value);
                            return std::format("{}({})", token.token_kind, literal);
                        } else {
                            return std::format("{}({})", token.token_kind, value);
                        }
                    },
                    token.data);

                return std::formatter<std::string>::format(std::format("{}", format), ctx);
            }
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for token of kind '{}'", static_cast<int>(token.token_kind));
    }
};
