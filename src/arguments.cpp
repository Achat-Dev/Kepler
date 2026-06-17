// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "arguments.hpp"
#include "log.hpp"

#include <cxxopts.hpp>
#include <string>
#include <vector>

namespace Kepler::Arguments {

    static std::string input_name;
    static std::string output_name;
    static std::vector<std::string> additional_files;
    static bool verbose;

    ArgumentParseResult parse(int argc, char* argv[]) {
        try {
            cxxopts::Options options("kepler", "The compiler for the kepler programming language");
            options.add_options()
                ("i,input", "The .kpl input file", cxxopts::value<std::string>())
                ("o,output", "The output file", cxxopts::value<std::string>())
                ("a,additional", "Additional C++ or object files, separated by ','", cxxopts::value<std::vector<std::string>>())
                ("v,verbose", "Enable verbose logging", cxxopts::value<bool>())
                ("h,help", "Print help")
            ;

            cxxopts::ParseResult result = options.parse(argc, argv);

            if (result.contains("help") && result.arguments().size() == 1) {
                std::cout << options.help();
                return ArgumentParseResult::HELP;
            }

            // Parse input
            const int count_input = result.count("input");
            if (count_input == 1) {
                input_name = result["input"].as<std::string>();
            }
            else {
                if (count_input > 1) {
                    log(LogStyle::ERROR, "[ Argument error ]", LogStyle::DEFAULT, ": input can only be specified once");
                }
                else {
                    log(LogStyle::ERROR, "[ Argument error ]", LogStyle::DEFAULT, ": no input file given");
                }
                return ArgumentParseResult::ERROR;
            }

            // Parse output
            const int count_output = result.count("output");
            if (count_output == 1) {
                output_name = result["output"].as<std::string>();
            }
            else {
                if (count_output > 1) {
                    log(LogStyle::ERROR, "[ Argument error ]", LogStyle::DEFAULT, ": output can only be specified once");
                }
                else {
                    log(LogStyle::ERROR, "[ Argument error ]", LogStyle::DEFAULT, ": no output file given");
                }
                return ArgumentParseResult::ERROR;
            }

            // Parse additional files
            if (result.count("additional")) {
                additional_files = result["additional"].as<std::vector<std::string>>();
            }

            verbose = result.contains("verbose");
        }
        catch (const cxxopts::exceptions::exception& e) {
            log(LogStyle::ERROR, "[ Argument error ]", LogStyle::DEFAULT, ": ", e.what());
            return ArgumentParseResult::ERROR;
        }

        return ArgumentParseResult::SUCCESS;
    }

    const std::string& get_input_file() {
        return input_name;
    }

    const std::string& get_output_file() {
        return output_name;
    }

    const std::vector<std::string>& get_additional_files() {
        return additional_files;
    }

    const bool should_log_verbose() {
        return verbose;
    }

}
