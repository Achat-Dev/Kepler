// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "file.hpp"

#include <filesystem>
#include <memory>
#include <string>

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
        if (!std::filesystem::exists(filename)) {
            return nullptr;
        }

        std::unique_ptr<File> file = std::unique_ptr<File>(new File());
        file->stream.open(filename);
        if (file->stream.is_open()) {
            return file;
        }
        return nullptr;
    }

}
