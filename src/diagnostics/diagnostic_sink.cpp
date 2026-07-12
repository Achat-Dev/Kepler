// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/severity.hpp"
#include "diagnostics/source_location.hpp"
#include "log.hpp"
#include <cstddef>
#include <fstream>
#include <print>
#include <string>

namespace kepler::diagnostics {

    void DiagnosticSink::report(DiagnosticCode code, const std::string& message) {
        report(code, message, "", {});
    }

    void DiagnosticSink::report(DiagnosticCode code, const std::string& message, const std::string& file_path, const SourceLocation& source_location) {
        const Severity severity = get_severity(code);
        switch (severity) {
            case Severity::Warning:
                warning_count++;
                break;
            case Severity::Error:
                error_count++;
                break;
            case Severity::Unsupported:
                error_count++;
                break;
            default:
                break;
        }

        diagnostics.push_back({code, message, file_path, source_location});
    }

    void DiagnosticSink::flush() {
        for (const Diagnostic& diagnostic : diagnostics) {
            const Severity severity = get_severity(diagnostic.code);

            if (diagnostic.file_path.empty()) {
                std::println("{}{}", severity, diagnostic.message);
                continue;
            }

            std::ifstream file_stream(diagnostic.file_path);
            if (!file_stream) {
                log::error("Failed to print diagnostics information, because how tf did the file '{}' get deleted while I was compiling it?", diagnostic.file_path);
                return;
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

            std::println("{}{}", severity, diagnostic.message);
            std::println("{}In '{}'", log::styling::indented, diagnostic.file_path);

            const std::string highlight_styling = get_severity_highlight(severity);
            const std::string prefix = std::format("{}At l.{} | ", log::styling::last_indented, line_number);
            const std::string ending = end_position_in_line < line.size() ? line.substr(end_position_in_line) : std::string(line.size() - end_position_in_line + 1, ' ');
            const std::string message = std::format("{}{}{}{}{}",
                line.substr(0, start_position_in_line),
                highlight_styling,
                line.substr(start_position_in_line, diagnostic.source_location.size),
                log::styling::reset,
                ending);
            std::println("{}{}", prefix, message);

            const std::string arrows(diagnostic.source_location.size, '^');
            std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + start_position_in_line, ' '), highlight_styling, arrows, log::styling::reset);
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

    std::string DiagnosticSink::get_severity_highlight(Severity severity) const {
        switch (severity) {
            case Severity::Note:
                return log::styling::bold;
            case Severity::Warning:
                return log::styling::combine(log::styling::bold, log::styling::yellow);
            case Severity::Error:
                return log::styling::combine(log::styling::bold, log::styling::red);
            case Severity::Unsupported:
                return log::styling::combine(log::styling::bold, log::styling::magenta);
            default:
                log::warning("No styling for severity '{}' found", static_cast<int>(severity));
                return "";
        }
    }

}
