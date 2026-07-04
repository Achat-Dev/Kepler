// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ansi_escape_codes.hpp"
#include "diagnostics/source_location.hpp"
#include "emergency.hpp"
#include "error_code.hpp"
#include "log.hpp"
#include <cstddef>
#include <expected>
#include <format>
#include <fstream>
#include <print>
#include <string>

namespace kepler::diagnostics {

    namespace {

        inline size_t strlen_utf8(const std::string& string) {
            size_t count = 0;
            for (unsigned char c : string) {
                if ((c & 0xC0) != 0x80) {
                    count++;
                }
            }
            return count;
        }

        inline void print_message(const std::string& file_path, const SourceLocation& source_location, const std::string& message_styling) {
            std::ifstream file_stream(file_path);
            if (!file_stream) {
                log(log_type::internal_error, "How tf did the file '", file_path, "' get deleted while I was compiling it?");
                emergency_exit();
            }

            std::string line;
            size_t line_number = 1;
            size_t current_position = 0;

            while (std::getline(file_stream, line)) {
                size_t line_length = line.size() + 1; // +1 because getline removes the '\n'
                current_position += line_length;
                if (current_position > source_location.position) {
                    current_position -= line_length; // Backtrack the current_position to the start of the line
                    break;
                }
                line_number += 1;
            }

            const size_t start_position_in_line = source_location.position - current_position;
            const size_t end_position_in_line = start_position_in_line + source_location.size;

            std::println("{}In file: '{}'", log_type::indented, file_path);

            const std::string prefix = std::format("{}At l.{} | ", log_type::last_indented, line_number);
            const std::string message = std::format("{}{}{}{}{}",
                line.substr(0, start_position_in_line),
                message_styling,
                line.substr(start_position_in_line, source_location.size),
                ansi_escape_codes::reset,
                line.substr(end_position_in_line));
            std::println("{}{}", prefix, message);

            const std::string arrows(source_location.size, '^');
            std::println("{}{}{}", std::string(strlen_utf8(prefix) + start_position_in_line, ' '), message_styling, arrows);
        }

    }

    template <typename... Args>
    std::unexpected<ErrorCode> error(const std::string& file_path, SourceLocation source_location, ErrorCode error_code, std::format_string<Args...> format, Args&&... args) {
        std::print("{}", error_code);
        std::println(format, args...);
        print_message(file_path, source_location, "\033[1;31m");
        return std::unexpected(error_code);
    }

}
