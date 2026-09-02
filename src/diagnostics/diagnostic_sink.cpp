// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "io/file.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <tuple>
#include <utility>

namespace kepler {

    void DiagnosticSink::report(DiagnosticCode code, std::string message, SourceLocation source_location) {
        KPL_ASSERT_THAT(source_location.size > 0, "Source location of reported diagnostic must have a size > 0");

        const DiagnosticSeverity severity = get_diagnostic_severity(code);
        switch (severity) {
            case DiagnosticSeverity::Note:
                break;
            case DiagnosticSeverity::Warning:
                warning_count++;
                break;
            case DiagnosticSeverity::Error:
                error_count++;
                break;
            case DiagnosticSeverity::Unsupported:
                error_count++;
                break;
        }

        diagnostics.emplace_back(code, std::move(message), source_location);
    }

    // TODO (fix): The source locations are sometimes off
    void DiagnosticSink::flush() {
        if (diagnostics.empty()) {
            return;
        }

        std::sort(diagnostics.begin(), diagnostics.end(), [](const SourceDiagnostic& a, const SourceDiagnostic& b) {
            const int severity_a = static_cast<int>(get_diagnostic_severity(a.code));
            const int severity_b = static_cast<int>(get_diagnostic_severity(b.code));
            return std::tie(a.source_location.file_id.value,
                       severity_a,
                       a.source_location.position) <
                   std::tie(b.source_location.file_id.value,
                       severity_b,
                       b.source_location.position);
        });

        for (const SourceDiagnostic& diagnostic : diagnostics) {
            const std::filesystem::path file_path = File::get_path_by_id(diagnostic.source_location.file_id);
            std::ifstream file_stream(file_path);
            if (!file_stream) {
                log::error("Failed to print diagnostics information for file '{}' because it doesn't exist", file_path.string());
                continue;
            }

            std::string line;
            size_t line_number = 1;
            size_t current_position = 0;

            while (std::getline(file_stream, line)) {
                size_t line_length = line.size() + 1; // +1 because getline removes the '\n'
                current_position += line_length;
                if (current_position > diagnostic.source_location.position) {
                    current_position -= line_length; // Backtrack the current_position to the start of the line
                    break;
                }
                line_number += 1;
            }

            const size_t start_position_in_line = diagnostic.source_location.position - current_position;
            const size_t end_position_in_line = start_position_in_line + diagnostic.source_location.size;

            const DiagnosticSeverity severity = get_diagnostic_severity(diagnostic.code);
            std::println("{}{}", severity, diagnostic.message);
            std::println("{}In '{}'", log::indented, file_path.string());

            const std::string highlight_styling = get_severity_highlight(severity);
            const std::string prefix = std::format("{}At l.{} | ", log::last_indented, line_number);
            std::string ending;
            if (end_position_in_line < line.size()) {
                ending = line.substr(end_position_in_line);
            } else {
                ending = std::string(line.size() - end_position_in_line + 1, ' ');
            }
            const std::string message = std::format("{}{}{}{}{}",
                line.substr(0, start_position_in_line),
                highlight_styling,
                line.substr(start_position_in_line, diagnostic.source_location.size),
                ansi_codes::reset,
                ending);
            std::println("{}{}", prefix, message);

            const std::string arrows(diagnostic.source_location.size, '^');
            std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + start_position_in_line, ' '), highlight_styling, arrows, ansi_codes::reset);
        }

        diagnostics.clear();
    }

    size_t DiagnosticSink::strlen_utf8(const std::string& string) const {
        size_t count = 0;
        for (unsigned char c : string) {
            if ((c & 0xC0) != 0x80) {
                count++;
            }
        }
        return count;
    }

    std::string DiagnosticSink::get_severity_highlight(DiagnosticSeverity severity) const {
        switch (severity) {
            case DiagnosticSeverity::Note:
                return ansi_codes::bold;
            case DiagnosticSeverity::Warning:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::yellow);
            case DiagnosticSeverity::Error:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::red);
            case DiagnosticSeverity::Unsupported:
                return ansi_codes::combine(ansi_codes::bold, ansi_codes::magenta);
        }

        KPL_ASSERT_UNREACHABLE("Missing styling implementation for severity '{}'", severity);
    }

}
