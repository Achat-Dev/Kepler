// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compilation_context.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include <cxxopts.hpp>
#include <expected>
#include <format>
#include <string>

namespace kepler {

    std::expected<CompilationContext, Diagnostic> parse_command_line_arguments(int argc, char* argv[]) {
        try {
            cxxopts::Options options("kepler", "The compiler for the kepler programming language");

            // clang-format off
            options.add_options()
                ("i,input", "The .kpl input file", cxxopts::value<std::string>())
                ("o,output", "The output file", cxxopts::value<std::string>())
                //("a,additional", "Additional C++ or object files, separated by ','", cxxopts::value<std::vector<std::string>>())
                ("v,verbose", "Enable verbose logging", cxxopts::value<bool>())
                ("h,help", "Print help");
            // clang-format on

            // Help option
            cxxopts::ParseResult parse_result = options.parse(argc, argv);
            if (parse_result.contains("help") && parse_result.arguments().size() == 1) {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::HelpRequested, .message = options.help()});
            }

            CompilationContext compilation_context;

            // Input option
            const int input_count = parse_result.count("input");
            if (input_count == 1) {
                compilation_context.input_file_path = parse_result["input"].as<std::string>();
            } else if (input_count > 1) {
                const std::string message = std::format("Input file (-i) can only be specified once, but was specified {} times", input_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::TooManyInputFiles, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoInputFile, .message = "Missing input file (-i)"});
            }

            // Output option
            const int output_count = parse_result.count("output");
            if (output_count == 1) {
                compilation_context.output_file_path = parse_result["output"].as<std::string>();
            } else if (input_count > 1) {
                const std::string message = std::format("Output file (-o) can only be specified once, but was specified {} times", output_count);
                return std::unexpected(Diagnostic{.code = DiagnosticCode::TooManyOutputFiles, .message = message});
            } else {
                return std::unexpected(Diagnostic{.code = DiagnosticCode::NoOutputFile, .message = "Missing output file (-o)"});
            }

            compilation_context.log_verbose = parse_result.contains("verbose");

            return compilation_context;
        } catch (const cxxopts::exceptions::exception& e) {
            return std::unexpected(Diagnostic{.code = DiagnosticCode::TooManyInputFiles, .message = e.what()});
        }
    }
}
