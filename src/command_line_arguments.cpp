// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "command_line_arguments.hpp"
#include "log.hpp"
#include <cxxopts.hpp>
#include <iostream>

namespace kepler {

    CommandLineArguments parse_command_line_arguments(int argc, char** argv) {
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

            cxxopts::ParseResult result = options.parse(argc, argv);
            if (result.contains("help") && result.arguments().size() == 1) {
                std::cout << options.help();
                exit(0);
            }

            CommandLineArguments arguments;

            // Parse input
            const int count_input = result.count("input");
            if (count_input == 1) {
                arguments.input_file_name = result["input"].as<std::string>();
            } else {
                if (count_input > 1) {
                    log(log_type::usage_error, "Input file can only be specified once");
                } else {
                    log(log_type::usage_error, "No input file given");
                }
                exit(1);
            }

            // Parse output
            const int count_output = result.count("output");
            if (count_output == 1) {
                arguments.output_file_name = result["output"].as<std::string>();
            } else {
                if (count_output > 1) {
                    log(log_type::usage_error, "Output file can only be specified once");
                } else {
                    log(log_type::usage_error, "No output file given");
                }
                exit(1);
            }

            log_config.should_log_verbose = result.contains("verbose");

            log_verbose("Successfully parsed command line arguments:\n",
                log_type::indented, "-i (input file name): '", arguments.input_file_name, "'\n",
                log_type::last_indented, "-o (output file name): '", arguments.output_file_name);

            return arguments;
        } catch (const cxxopts::exceptions::exception& e) {
            log(log_type::usage_error, e.what());
            exit(1);
        }
    }

}
