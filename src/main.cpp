#include "arguments.hpp"
#include "compiler.hpp"
#include "log.hpp"

int main(int argc, char* argv[]) {
    const Kepler::Arguments::ArgumentParseResult argument_parse_result = Kepler::Arguments::parse(argc, argv);
    if (argument_parse_result == Kepler::Arguments::ArgumentParseResult::ERROR) {
        return 1;
    }
    if (argument_parse_result == Kepler::Arguments::ArgumentParseResult::HELP) {
        return 0;
    }

    if (!Kepler::Compiler::compile_file()) {
        Kepler::log(Kepler::LogStyle::ERROR, "Failed to compile file '", Kepler::Arguments::get_input_file(), "' and write it to '", Kepler::Arguments::get_output_file(), '\'');
        return 1;
    }

	return 0;
}
