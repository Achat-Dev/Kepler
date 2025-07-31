#pragma once

#include <fstream>
#include <memory>
#include <string>

namespace Kepler {

    class File {
    private:
        std::ifstream stream;
        int current_line_number;

        File(): current_line_number(1) {}

    public:
        char read_next_char();
        int peek();
        void close();
        int get_current_line_number() const;

        static std::unique_ptr<File> create(const std::string& filename);
    };

}
