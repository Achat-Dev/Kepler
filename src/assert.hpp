// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "log.hpp"
#include <cstdlib>
#include <format>
#include <print>
#include <utility>

#ifndef NDEBUG
#define KPL_ASSERT(expr, message, ...)                                                             \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            kepler::detail::assert(#expr, __FILE__, __LINE__, message __VA_OPT__(, ) __VA_ARGS__); \
        }                                                                                          \
    } while (false)
#else
#define KPL_ASSERT(expr, message, ...) ((void)0)
#endif

namespace kepler::detail {

    template <typename... Args>
    [[noreturn]]
    void assert(const char* expression, const char* file_path, int line_number, std::format_string<Args...> format, Args&&... args) {
        std::println("{}[ Internal emergency, everybody panic ]{}: {}\n{}(This means assertion failed)\n{}'{}': l. {} | '{}'",
            log::styling::bold,
            log::styling::reset,
            std::format(format, std::forward<Args>(args)...),
            log::styling::indented,
            log::styling::last_indented,
            file_path,
            line_number,
            expression);
        std::abort();
    }

}
