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

namespace kepler::diagnostics {

    enum class WarningCode {
        LexerMultilineCommentNotClosed = 101,
    };

}

template <>
struct std::formatter<kepler::diagnostics::WarningCode> : std::formatter<std::string> {
    auto format(const kepler::diagnostics::WarningCode& warning_code, std::format_context& ctx) const {
        switch (warning_code) {
            case kepler::diagnostics::WarningCode::LexerMultilineCommentNotClosed:
                return std::formatter<std::string>::format(std::format("{}{}[ Lexing warning ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_yellow,
                                                               kepler::log::styling::reset),
                    ctx);
            default:
                kepler::log::warning("Missing format implementation for warning code '{}'", static_cast<int>(warning_code));
                return std::formatter<std::string>::format(std::format(""), ctx);
        }
    }
};
