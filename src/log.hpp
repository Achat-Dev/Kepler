// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <iostream>

#define ANSI_RESET "\033[0m"
#define ANSI_VERBOSE "\033[2m"
#define ANSI_WARNING "\033[1;30;43m"
#define ANSI_ERROR "\033[1;30;41m"
#define ANSI_UNSUPPORTED "\033[1;30;45m"

namespace kepler {

    struct LogConfig {
        bool should_log_verbose = false;
    };

    inline LogConfig log_config;

    namespace log_style {
        inline constexpr char bold[] = "\033[1m";
        inline constexpr char italic[] = "\033[3m";
        inline constexpr char reset[] = ANSI_RESET;
        inline constexpr char warning[] = ANSI_WARNING;
        inline constexpr char error[] = ANSI_ERROR;
        inline constexpr char unsupported[] = ANSI_UNSUPPORTED;
    }

    namespace log_type {
        inline constexpr char lexing_warning[] = ANSI_WARNING "[ Lexing warning ]" ANSI_RESET ": ";

        inline constexpr char usage_error[] = ANSI_ERROR "[ Usage error ]" ANSI_RESET ": ";
        inline constexpr char io_error[] = ANSI_ERROR "[ I/O error ]" ANSI_RESET ": ";
        inline constexpr char lexing_error[] = ANSI_ERROR "[ Lexing error ]" ANSI_RESET ": ";
        inline constexpr char parsing_error[] = ANSI_ERROR "[ Parsing error ]" ANSI_RESET ": ";
        inline constexpr char compile_error[] = ANSI_ERROR "[ Compile error ]" ANSI_RESET ": ";

        inline constexpr char internal_lexing_warning[] = ANSI_WARNING "[ Internal lexing warning ]" ANSI_RESET ": ";
        inline constexpr char unsupported[] = ANSI_UNSUPPORTED "[ Unpaid developer error ]" ANSI_RESET ": ";
        inline constexpr char internal_error[] = ANSI_UNSUPPORTED "[ Internal error, everybody panic ]" ANSI_RESET ": ";

        inline constexpr char indented[] = "  \u251C\u2500 ";
        inline constexpr char last_indented[] = "  \u2514\u2500 ";
    }

    template <typename T>
    inline void log(const T& t) {
        std::cout << t << ANSI_RESET << std::endl;
    }

    template <typename T, typename... Args>
    inline void log(const T& t, const Args&... args) {
        std::cout << t;
        log(args...);
    }

    template <typename... Args>
    inline void log_verbose(const Args&... args) {
        if (log_config.should_log_verbose) {
            log(ANSI_VERBOSE, "[ Verbose ]: ", args...);
        }
    }

    template <typename... Args>
    inline void log_verbose_no_prefix(const Args&... args) {
        if (log_config.should_log_verbose) {
            log(ANSI_VERBOSE, args...);
        }
    }

}
