// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstddef>
#include <format>
#include <print>
#include <string>
#include <utility>

namespace kepler::log {

    namespace styling {

        inline constexpr char reset[] = "\033[0m";
        inline constexpr char indented[] = "  \u251C\u2500 ";
        inline constexpr char last_indented[] = "  \u2514\u2500 ";

        // Styles
        inline constexpr char bold[] = "\033[1m";
        inline constexpr char dim[] = "\033[2m";
        inline constexpr char italic[] = "\033[3m";

        // Text colours
        inline constexpr char black[] = "\033[30m";
        inline constexpr char red[] = "\033[31m";
        inline constexpr char green[] = "\033[32m";
        inline constexpr char yellow[] = "\033[33m";
        inline constexpr char blue[] = "\033[34m";
        inline constexpr char magenta[] = "\033[35m";
        inline constexpr char cyan[] = "\033[36m";
        inline constexpr char white[] = "\033[37m";

        // Background colours
        inline constexpr char bg_black[] = "\033[40m";
        inline constexpr char bg_red[] = "\033[41m";
        inline constexpr char bg_green[] = "\033[42m";
        inline constexpr char bg_yellow[] = "\033[43m";
        inline constexpr char bg_blue[] = "\033[44m";
        inline constexpr char bg_magenta[] = "\033[45m";
        inline constexpr char by_cyan[] = "\033[46m";
        inline constexpr char bg_white[] = "\033[47m";

        template <typename... Args>
        std::string combine(Args&&... args) {
            constexpr size_t arg_count = sizeof...(args);
            std::string format;
            for (size_t i = 0; i < arg_count; i++) {
                format += "{}";
            }
            return std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
        }

    }

    struct LogConfig {
        bool should_log_verbose;
    };

    inline LogConfig config;

    template <typename... Args>
    void verbose(std::format_string<Args...> format, Args&&... args) {
        std::println("{}[ Verbose ]: {}{}", styling::dim, std::format(format, std::forward<Args>(args)...), styling::reset);
    }

    template <typename... Args>
    void verbose_no_prefix(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}{}", styling::dim, std::format(format, std::forward<Args>(args)...), styling::reset);
    }

    template <typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) {
        std::println(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}[ Internal warning ]{}: {}", styling::bold, styling::yellow, styling::reset, std::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}[ Internal error ]{}: {}", styling::bold, styling::red, styling::reset, std::format(format, std::forward<Args>(args)...));
    }

}
