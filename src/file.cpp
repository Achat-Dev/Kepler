#include <memory>
#include <string>

#include "file.hpp"

namespace Kepler {

    char File::read_next_char() {
        char c;
        stream.get(c);
        if (c == '\n') {
            current_line_number++;
        }
        return c;
    }

    int File::peek() {
        return stream.peek();
    }

    void File::close() {
        stream.close();
    }

    int File::get_current_line_number() const {
        return current_line_number;
    }

    std::unique_ptr<File> File::create(const std::string& filename) {
        std::unique_ptr<File> file = std::unique_ptr<File>(new File());
        file->stream.open(filename);
        if (file->stream.is_open()) {
            return file;
        }
        return nullptr;
    }

}
