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

    enum class OperatorType {
        Plus,
        Minus,
        Multiplication,
        Division,
        LessThan,
        GreaterThan,
        Equals,
        NotEquals,
        LessEquals,
        GreaterEquals,
    };

}

template <>
struct std::formatter<kepler::lexer::OperatorType> : std::formatter<std::string> {
    auto format(const kepler::lexer::OperatorType& operator_type, std::format_context& ctx) const {
        switch (operator_type) {
            case kepler::lexer::OperatorType::Plus:
                return std::formatter<std::string>::format("+", ctx);
            case kepler::lexer::OperatorType::Minus:
                return std::formatter<std::string>::format("-", ctx);
            case kepler::lexer::OperatorType::Multiplication:
                return std::formatter<std::string>::format("*", ctx);
            case kepler::lexer::OperatorType::Division:
                return std::formatter<std::string>::format("/", ctx);
            case kepler::lexer::OperatorType::LessThan:
                return std::formatter<std::string>::format("<", ctx);
            case kepler::lexer::OperatorType::GreaterThan:
                return std::formatter<std::string>::format(">", ctx);
            case kepler::lexer::OperatorType::Equals:
                return std::formatter<std::string>::format("==", ctx);
            case kepler::lexer::OperatorType::NotEquals:
                return std::formatter<std::string>::format("!=", ctx);
            case kepler::lexer::OperatorType::LessEquals:
                return std::formatter<std::string>::format("<=", ctx);
            case kepler::lexer::OperatorType::GreaterEquals:
                return std::formatter<std::string>::format(">=", ctx);
            default:
                kepler::log::warning("Missing format implementation for operator type '{}'", static_cast<int>(operator_type));
                return std::formatter<std::string>::format(std::format("{}", static_cast<int>(operator_type)), ctx);
        }
    }
};
