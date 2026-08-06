// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/ansi_codes.hpp"
#include <format>
#include <print>
#include <utility>

namespace kepler::log {

    struct LogConfig {
        bool should_log_verbose;
    };

    inline LogConfig config;
    inline constexpr char indented[] = "  \u251C\u2500 ";
    inline constexpr char last_indented[] = "  \u2514\u2500 ";

    template <typename... Args>
    void verbose(std::format_string<Args...> format, Args&&... args) {
        if (config.should_log_verbose) {
            std::println("{}[ Verbose ]: {}{}", ansi_codes::dim, std::format(format, std::forward<Args>(args)...), ansi_codes::reset);
        }
    }

    template <typename... Args>
    void verbose_no_prefix(std::format_string<Args...> format, Args&&... args) {
        if (config.should_log_verbose) {
            std::println("{}{}{}", ansi_codes::dim, std::format(format, std::forward<Args>(args)...), ansi_codes::reset);
        }
    }

    template <typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) {
        std::println(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}[ Internal warning ]{}: {}", ansi_codes::bold, ansi_codes::yellow, ansi_codes::reset, std::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}[ Internal error ]{}: {}", ansi_codes::bold, ansi_codes::red, ansi_codes::reset, std::format(format, std::forward<Args>(args)...));
    }

}
