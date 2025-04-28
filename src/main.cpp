#include <cstdio>

#include "compiler.hpp"
#include "log.hpp"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        Kepler::log(Kepler::LogStyle::ERROR, "[ Error ]", Kepler::LogStyle::DEFAULT, ": no filename given");
        return 1;
    }
    if (argc <= 2) {
        Kepler::log(Kepler::LogStyle::ERROR, "[ Error ]", Kepler::LogStyle::DEFAULT, ": no output name given");
        return 1;
    }

    const char* filename = argv[1];
    const char* outname = argv[2];
    if (!Kepler::Compiler::compile_file(filename, outname)) {
        Kepler::log(Kepler::LogStyle::ERROR, "Failed to compile file '", filename, "' and write it to '", outname, '\'');
        return 1;
    }

	return 0;
}
