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
#include <source_location>
#include <string_view>
#include <utility>

namespace kepler::assert {

    namespace {
        inline void print(const std::string& message, const std::source_location& location) {
            std::string_view file_path(location.file_name());
            const size_t prefix_position = file_path.find("src");
            if (prefix_position != std::string_view::npos) {
                file_path.remove_prefix(prefix_position);
                std::println("{}{}[ Assertion failed ]{}: {}\n{}At '{}' at l. '{}'",
                    ansi_codes::bold,
                    ansi_codes::red,
                    ansi_codes::reset,
                    message,
                    log::last_indented,
                    file_path,
                    location.line());
            } else {
                std::println("{}{}[ Assertion failed ]{}: {}",
                    ansi_codes::bold,
                    ansi_codes::red,
                    ansi_codes::reset,
                    message);
            }
        }
    }

    inline void that(bool condition, const std::string& message = "", const std::source_location& location = std::source_location::current()) {
        if (!condition) {
            print(message, location);
            std::abort();
        }
    }

    [[noreturn]]
    inline void unreachable(const std::string& message, const std::source_location& location = std::source_location::current()) {
        print(message, location);
        std::unreachable();
    }

    template <typename T>
    inline void not_nullptr(const T* ptr, const std::source_location& location = std::source_location::current()) {
        if (ptr == nullptr) {
            print("nullptr violation", location);
            std::abort();
        }
    }

}
