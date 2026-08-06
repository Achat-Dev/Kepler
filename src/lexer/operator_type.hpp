// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/string_pool.hpp"
#include <cassert>
#include <format>
#include <string>
#include <utility>

namespace kepler {

    // TODO: Add unary operators
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

    StringId get_operator_name_id(OperatorType operator_type);

}

template <>
struct std::formatter<kepler::OperatorType> : std::formatter<std::string> {
    auto format(const kepler::OperatorType& operator_type, std::format_context& ctx) const {
        switch (operator_type) {
            case kepler::OperatorType::Plus:
                return std::formatter<std::string>::format("+", ctx);
            case kepler::OperatorType::Minus:
                return std::formatter<std::string>::format("-", ctx);
            case kepler::OperatorType::Multiplication:
                return std::formatter<std::string>::format("*", ctx);
            case kepler::OperatorType::Division:
                return std::formatter<std::string>::format("/", ctx);
            case kepler::OperatorType::LessThan:
                return std::formatter<std::string>::format("<", ctx);
            case kepler::OperatorType::GreaterThan:
                return std::formatter<std::string>::format(">", ctx);
            case kepler::OperatorType::Equals:
                return std::formatter<std::string>::format("==", ctx);
            case kepler::OperatorType::NotEquals:
                return std::formatter<std::string>::format("!=", ctx);
            case kepler::OperatorType::LessEquals:
                return std::formatter<std::string>::format("<=", ctx);
            case kepler::OperatorType::GreaterEquals:
                return std::formatter<std::string>::format(">=", ctx);
        }

        assert(false && "Missing format implementation for operator type");
        std::unreachable();
    }
};
