#pragma once

#include <fstream>
#include <memory>

namespace Kepler {

    class File {
    private:
        std::ifstream stream;
        uint current_line_number;

        File(): current_line_number(1) {}

    public:
        const char read_next_char();
        const int peek();
        const void close();
        const uint get_current_line_number() const;

        static std::unique_ptr<File> create(const char* filename);
    };

}
