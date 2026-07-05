// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/source_location.hpp"
#include "log.hpp"
#include <cstddef>
#include <format>
#include <fstream>
#include <print>
#include <string>

namespace kepler::diagnostics {

    namespace {
        size_t strlen_utf8(const std::string& string) {
            size_t count = 0;
            for (unsigned char c : string) {
                if ((c & 0xC0) != 0x80) {
                    count++;
                }
            }
            return count;
        }
    }

    void print_source_code_diagnostics(const std::string& file_path, const SourceLocation& source_location, const std::string& highlight_styling) {
        std::ifstream file_stream(file_path);
        if (!file_stream) {
            log::error("Failed to print diagnostics information, because how tf did the file '{}' get deleted while I was compiling it?", file_path);
            return;
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

        std::println("{}In file: '{}'", log::styling::indented, file_path);

        const std::string prefix = std::format("{}At l.{} | ", log::styling::last_indented, line_number);
        const std::string message = std::format("{}{}{}{}{}",
            line.substr(0, start_position_in_line),
            highlight_styling,
            line.substr(start_position_in_line, source_location.size),
            log::styling::reset,
            line.substr(end_position_in_line));
        std::println("{}{}", prefix, message);

        const std::string arrows(source_location.size, '^');
        std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + start_position_in_line, ' '), highlight_styling, arrows, log::styling::reset);
    }

}
