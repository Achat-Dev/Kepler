// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "diagnostics/warning_code.hpp"
#include "error_code.hpp"
#include "log.hpp"
#include <expected>
#include <format>
#include <print>
#include <string>
#include <utility>

namespace kepler::diagnostics {

    void print_source_code_diagnostics(const std::string& file_path, const SourceLocation& source_location, const std::string& highlight_styling);

    template <typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) {
        std::println(format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(const std::string& file_path, SourceLocation source_location, WarningCode warning_code, std::format_string<Args...> format, Args&&... args) {
        std::print("{}", warning_code);
        std::println(format, std::forward<Args>(args)...);
        print_source_code_diagnostics(file_path, source_location, log::styling::combine(log::styling::bold, log::styling::yellow));
    }

    template <typename... Args>
    std::unexpected<ErrorCode> error(ErrorCode error_code, std::format_string<Args...> format, Args&&... args) {
        std::print("{}", error_code);
        std::println(format, std::forward<Args>(args)...);
        return std::unexpected(error_code);
    }

    template <typename... Args>
    std::unexpected<ErrorCode> error(const std::string& file_path, SourceLocation source_location, ErrorCode error_code, std::format_string<Args...> format, Args&&... args) {
        std::print("{}", error_code);
        std::println(format, std::forward<Args>(args)...);
        if (source_location.position != 0 || source_location.size != 0) {
            print_source_code_diagnostics(file_path, source_location, log::styling::combine(log::styling::bold, log::styling::bg_red));
        }
        return std::unexpected(error_code);
    }

}
