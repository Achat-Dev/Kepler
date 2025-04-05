#include <cstdio>
#include <iostream>

#include "compiler.hpp"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Error: no filename given" << std::endl;
        return 1;
    }
    if (argc <= 2) {
        std::cout << "Error: no output name given" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    const char* outname = argv[2];
    if (!Kepler::Compiler::compile_file(filename, outname)) {
        std::cout << "Failed to compile file '" << filename << "'" << std::endl;
        return 1;
    }

	return 0;
}
