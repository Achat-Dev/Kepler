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
#include "io/file_manager.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <print>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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

    auto DiagnosticSink::find_line_info(const std::vector<LineInfo>& line_infos, uint32_t position) {
        auto it = std::lower_bound(line_infos.begin(),
            line_infos.end(),
            position,
            [](const LineInfo& line_info, uint32_t position) {
                return line_info.start_position < position;
            });

        if (it != line_infos.begin() && (it == line_infos.end() || it->start_position != position)) {
            it--;
        }
        return it;
    }

    // TODO (bug): Line prefixes don't pad the line number,
    // so if the magnitude of the line number changes during a multiline diagnostic, the lines are not aligned
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

        File* file = FileManager::get().lookup(diagnostics[0].source_location.file_id);
        std::vector<LineInfo> line_infos = get_line_infos(file);
        for (const SourceDiagnostic& diagnostic : diagnostics) {
            if (diagnostic.source_location.file_id != file->id) {
                file = FileManager::get().lookup(diagnostic.source_location.file_id);
                line_infos = get_line_infos(file);
            }

            // Print diagnsotic
            const DiagnosticSeverity severity = get_diagnostic_severity(diagnostic.code);
            std::println("{}{}", severity, diagnostic.message);
            std::println("{}In '{}'", log::indented, file->path.string());

            // Print lines
            const std::string highlight_styling = get_severity_highlight(severity);
            const uint32_t diagnostic_end_position = diagnostic.source_location.position + diagnostic.source_location.size;
            auto start_it = find_line_info(line_infos, diagnostic.source_location.position);
            auto end_it = find_line_info(line_infos, diagnostic_end_position);
            auto it = start_it;
            do {
                uint32_t line_end_position = it->start_position + it->size;
                if (diagnostic.source_location.position < it->start_position) {
                    // There were previous lines
                    if (diagnostic_end_position > line_end_position) {
                        // There are more lines coming afterwards
                        const std::string prefix = std::format("{}At l.{} | ", log::indented, it->line_number);
                        const std::string line = file->content.substr(it->start_position, it->size);
                        std::print("{}{}{}{}", prefix, highlight_styling, line, ansi_codes::reset);

                        if (it->size > 1) {
                            const std::string arrows(strlen_utf8(line) - 1, '^'); // -1 because of the newline character
                            std::println("{}{}{}{}", std::string(strlen_utf8(prefix), ' '), highlight_styling, arrows, ansi_codes::reset);
                        }
                    } else {
                        // This is the last line
                        const std::string prefix = std::format("{}At l.{} | ", log::last_indented, it->line_number);
                        const uint32_t diagnostic_size = diagnostic_end_position - it->start_position;
                        const uint32_t line_end_size = line_end_position - diagnostic_end_position;
                        const std::string diagnostic_string = file->content.substr(it->start_position, diagnostic_size);
                        std::print("{}{}{}{}{}",
                            prefix,
                            highlight_styling,
                            diagnostic_string,
                            ansi_codes::reset,
                            file->content.substr(diagnostic_end_position, line_end_size));

                        const uint32_t leading_space_count = diagnostic_string.find_first_not_of(" \t");
                        const std::string arrows(strlen_utf8(diagnostic_string) - leading_space_count, '^');
                        std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + leading_space_count, ' '), highlight_styling, arrows, ansi_codes::reset);
                    }
                } else {
                    // This is the first line
                    const uint32_t line_start_size = diagnostic.source_location.position - start_it->start_position;
                    const std::string line_start = file->content.substr(start_it->start_position, line_start_size);

                    if (diagnostic_end_position > line_end_position) {
                        // There are more lines coming afterwards
                        const std::string prefix = std::format("{}At l.{} | ", log::indented, it->line_number);
                        const uint32_t line_end_size = line_end_position - diagnostic.source_location.position;
                        const std::string diagnostic_string = file->content.substr(diagnostic.source_location.position, line_end_size);
                        std::print("{}{}{}{}", prefix, line_start, highlight_styling, diagnostic_string, ansi_codes::reset);

                        const std::string arrows(strlen_utf8(diagnostic_string) - 1, '^'); // -1 because of the newline character
                        std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + strlen_utf8(line_start), ' '), highlight_styling, arrows, ansi_codes::reset);
                    } else {
                        // This is also the last line, so it's the only line
                        const std::string prefix = std::format("{}At l.{} | ", log::last_indented, it->line_number);
                        const uint32_t line_end_size = (start_it->start_position + start_it->size) - diagnostic_end_position;
                        const std::string diagnostic_string = file->content.substr(diagnostic.source_location.position, diagnostic.source_location.size);
                        std::print("{}{}{}{}{}{}",
                            prefix,
                            line_start,
                            highlight_styling,
                            diagnostic_string,
                            ansi_codes::reset,
                            file->content.substr(diagnostic_end_position, line_end_size));

                        std::string arrows = "";
                        if (diagnostic_string.ends_with('\n')) {
                            arrows = std::string(strlen_utf8(diagnostic_string) - 1, '^'); // -1 because of newline character
                        } else {
                            arrows = std::string(strlen_utf8(diagnostic_string), '^');
                        }
                        std::println("{}{}{}{}", std::string(strlen_utf8(prefix) + strlen_utf8(line_start), ' '), highlight_styling, arrows, ansi_codes::reset);
                    }
                }
                it++;
            } while (it != std::next(end_it));
        }

        diagnostics.clear();
    }

}
