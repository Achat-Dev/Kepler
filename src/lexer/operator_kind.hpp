// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <string>

namespace kepler {

    // TODO (improvement): Add unary operators
    // Need to check if that is worth it when more unary operators exist
    enum class OperatorKind {
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

    StringId get_operator_identifier_id(OperatorKind operator_kind);

}

template <>
struct std::formatter<kepler::OperatorKind> : std::formatter<std::string> {
    auto format(const kepler::OperatorKind& operator_kind, std::format_context& ctx) const {
        switch (operator_kind) {
            case kepler::OperatorKind::Plus:
                return std::formatter<std::string>::format("+", ctx);
            case kepler::OperatorKind::Minus:
                return std::formatter<std::string>::format("-", ctx);
            case kepler::OperatorKind::Multiplication:
                return std::formatter<std::string>::format("*", ctx);
            case kepler::OperatorKind::Division:
                return std::formatter<std::string>::format("/", ctx);
            case kepler::OperatorKind::LessThan:
                return std::formatter<std::string>::format("<", ctx);
            case kepler::OperatorKind::GreaterThan:
                return std::formatter<std::string>::format(">", ctx);
            case kepler::OperatorKind::Equals:
                return std::formatter<std::string>::format("==", ctx);
            case kepler::OperatorKind::NotEquals:
                return std::formatter<std::string>::format("!=", ctx);
            case kepler::OperatorKind::LessEquals:
                return std::formatter<std::string>::format("<=", ctx);
            case kepler::OperatorKind::GreaterEquals:
                return std::formatter<std::string>::format(">=", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for operator kind '{}'", static_cast<int>(operator_kind));
    }
};
