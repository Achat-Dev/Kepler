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
        inline constexpr char BOLD[] = "\033[1m";
        inline constexpr char ITALIC[] = "\033[3m";
        inline constexpr char RESET[] = ANSI_RESET;
        inline constexpr char WARNING[] = ANSI_WARNING;
        inline constexpr char ERROR[] = ANSI_ERROR;
        inline constexpr char UNSUPPORTED[] = ANSI_UNSUPPORTED;
    }

    namespace log_type {
        inline constexpr char LEXING_WARNING[] = ANSI_WARNING "[ Lexing warning ]" ANSI_RESET ": ";

        inline constexpr char USAGE_ERROR[] = ANSI_ERROR "[ Usage error ]" ANSI_RESET ": ";
        inline constexpr char IO_ERROR[] = ANSI_ERROR "[ I/O error ]" ANSI_RESET ": ";
        inline constexpr char LEXING_ERROR[] = ANSI_ERROR "[ Lexing error ]" ANSI_RESET ": ";
        inline constexpr char COMPILE_ERROR[] = ANSI_ERROR "[ Compile error ]" ANSI_RESET ": ";

        inline constexpr char INTERNAL_LEXING_WARNING[] = ANSI_WARNING "[ Internal lexing warning ]" ANSI_RESET ": ";
        inline constexpr char UNSUPPORTED[] = ANSI_UNSUPPORTED "[ Unpaid developer error ]" ANSI_RESET ": ";

        inline constexpr char INDENTED[] = "\u251C\u2500\u2500\u2500 ";
        inline constexpr char LAST_INDENTED[] = "\u2514\u2500\u2500\u2500 ";
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
