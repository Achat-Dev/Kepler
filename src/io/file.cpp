// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "io/file.hpp"
#include "utils/assert.h"
#include <cstdint>
#include <string>
#include <vector>

namespace kepler {

    std::vector<LineInfo> get_line_infos(const File* file) {
        KPL_ASSERT_NOT_NULLPTR(file);
        uint32_t line_number = 1;
        uint32_t current_position = 0;
        uint32_t last_line_start_position = 0;
        std::vector<LineInfo> result;

        while (current_position < file->content.size()) {
            if (file->content[current_position] == '\n') {
                const uint32_t line_size = (current_position + 1) - last_line_start_position; // +1 because the newline also counts towards the size
                result.push_back({.line_number = line_number, .start_position = last_line_start_position, .size = line_size});
                line_number += 1;
                current_position += 1;
                last_line_start_position = current_position;
            } else {
                current_position += 1;
            }
        }

        // Last line didn't end with a newline, so add the final line
        if (last_line_start_position != current_position) {
            const uint32_t line_size = current_position - last_line_start_position;
            result.push_back({.line_number = line_number, .start_position = last_line_start_position, .size = line_size});
        }

        return result;
    }

}
