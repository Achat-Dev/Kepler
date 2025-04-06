#include <iostream>
#include <memory>

#include "file.hpp"

namespace Kepler {

    const char File::read_next_char() {
        char c;
        stream.get(c);
        if (c == '\n') {
            current_line_number++;
            std::cout << "--- line break detected " << current_line_number << " ---";
        }
        return c;
    }

    const int File::peek() {
        return stream.peek();
    }

    const void File::close() {
        stream.close();
    }

    const uint File::get_current_line_number() const {
        return current_line_number;
    }

    std::unique_ptr<File> File::create(const char *filename) {
        std::unique_ptr<File> file = std::unique_ptr<File>(new File());
        file->stream.open(filename);
        if (file->stream.is_open()) {
            return file;
        }
        return nullptr;
    }

}
