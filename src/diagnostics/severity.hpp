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

    enum class Severity {
        Note,
        Warning,
        Error,
        Unsupported,
    };

}

template <>
struct std::formatter<kepler::diagnostics::Severity> : std::formatter<std::string> {
    auto format(const kepler::diagnostics::Severity& severity, std::format_context& ctx) const {
        switch (severity) {
            case kepler::diagnostics::Severity::Note:
                return std::formatter<std::string>::format(std::format("{}[ Note ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::diagnostics::Severity::Warning:
                return std::formatter<std::string>::format(std::format("{}{}[ Warning ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_yellow,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::diagnostics::Severity::Error:
                return std::formatter<std::string>::format(std::format("{}{}[ Error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::diagnostics::Severity::Unsupported:
                return std::formatter<std::string>::format(std::format("{}{}[ Unpaid developer ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_magenta,
                                                               kepler::log::styling::reset),
                    ctx);
            default:
                kepler::log::warning("Missing format implementation for severity '{}'", static_cast<int>(severity));
                return std::formatter<std::string>::format(std::format(""), ctx);
        }
    }
};
