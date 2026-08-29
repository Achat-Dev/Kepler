// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/ansi_codes.hpp"
#include "utils/log.hpp"
#include <cstddef>
#include <cstdlib>
#include <format>
#include <print>
#include <string_view>
#include <utility>

namespace kepler::internal {

    template <typename... Args>
    void print_assertion(const char* file_path, int line_number, std::format_string<Args...> format, Args&&... args) {
        std::string_view relative_file_path(file_path);
        const size_t prefix_position = relative_file_path.find("src");
        if (prefix_position != std::string_view::npos) {
            relative_file_path.remove_prefix(prefix_position);
            std::println("{}{}[ Assertion failed ]{}: {}\n{}In '{}' at l. {}",
                ansi_codes::bold,
                ansi_codes::red,
                ansi_codes::reset,
                std::format(std::move(format), std::forward<Args>(args)...),
                log::last_indented,
                relative_file_path,
                line_number);
        } else {
            std::println("{}{}[ Assertion failed ]{}: {}",
                ansi_codes::bold,
                ansi_codes::red,
                ansi_codes::reset,
                std::format(std::move(format), std::forward<Args>(args)...));
        }
    }

}

// Uncomment the next line to disable assertions
// #define KPL_NO_ASSERT

#ifndef KPL_NO_ASSERT
#define KPL_ASSERT_THAT(expr, message, ...)                                                            \
    do {                                                                                               \
        if (!(expr)) {                                                                                 \
            kepler::internal::print_assertion(__FILE__, __LINE__, message __VA_OPT__(, ) __VA_ARGS__); \
            std::abort();                                                                              \
        }                                                                                              \
    } while (false)

#define KPL_ASSERT_NOT_NULLPTR(ptr)                                                     \
    do {                                                                                \
        if ((ptr) == nullptr) {                                                         \
            kepler::internal::print_assertion(__FILE__, __LINE__, "nullptr violation"); \
            std::abort();                                                               \
        }                                                                               \
    } while (false)

#define KPL_ASSERT_NOT_POISONED(node, operation_description) \
    do {                                                     \
        if ((node)->node_type == ASTNodeType::Poison) {      \
            kepler::internal::print_assertion(__FILE__,      \
                __LINE__,                                    \
                "{} must be unpoisoned for {}",              \
                (node)->node_type,                           \
                operation_description);                      \
            std::abort();                                    \
        }                                                    \
    } while (false)

#define KPL_ASSERT_HOLDS_ALTERNATIVE(data, type, prefix)          \
    do {                                                          \
        if (!std::holds_alternative<type>((data))) {              \
            kepler::internal::print_assertion(__FILE__,           \
                __LINE__,                                         \
                "{} must contain '" #type "', but contains '{}'", \
                prefix,                                           \
                typeid(data).name());                             \
            std::abort();                                         \
        }                                                         \
    } while (false)

#define KPL_ASSERT_UNREACHABLE(message, ...)                                                       \
    do {                                                                                           \
        kepler::internal::print_assertion(__FILE__, __LINE__, message __VA_OPT__(, ) __VA_ARGS__); \
        std::unreachable();                                                                        \
    } while (false)

#else
#define KPL_ASSERT_THAT(expr, message, ...) ((void)0)
#define KPL_ASSERT_NOT_NULLPTR(ptr) ((void)0)
#define KPL_ASSERT_NOT_POISONED(node, operation_description) ((void)0)
#define KPL_ASSERT_HOLDS_ALTERNATIVE(data, type, message) ((void)0)
#define KPL_ASSERT_UNREACHABLE(message, ...) std::unreachable()
#endif
