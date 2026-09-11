// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/log.hpp"
#include <cstddef>
#include <string>

namespace kepler {

    std::size_t strlen_utf8(const std::string& string) {
        size_t count = 0;
        for (size_t i = 0; i < string.size();) {
            unsigned char c = string[i];
            if (c < 0x80) {
                i += 1; // ASCII
            } else if ((c & 0xE0) == 0xC0) {
                i += 2; // 2-byte UTF-8
            } else if ((c & 0xF0) == 0xE0) {
                i += 3; // 3-byte UTF-8
            } else if ((c & 0xF8) == 0xF0) {
                i += 4; // 4-byte UTF-8
            } else {
                log::error("Invalid UTF8 character '{}' when trying to print diagnostic, diagnostic arrows might be off", c);
                return 0;
            }
            count += 1;
        }
        return count;
    }

}
