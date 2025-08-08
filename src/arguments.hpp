#pragma once

#include <string>
#include <vector>
namespace Kepler::Arguments {

    enum class ArgumentParseResult {
        HELP,
        ERROR,
        SUCCESS,
    };

    ArgumentParseResult parse(int argc, char* argv[]);

    const std::string& get_input_file();
    const std::string& get_output_file();
    const std::vector<std::string>& get_additional_files();
    const bool should_log_verbose();

}
