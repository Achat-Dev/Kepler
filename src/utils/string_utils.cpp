// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/assert.h"
#include "utils/log.hpp"
#include <cctype>
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

    bool is_utf8_character(unsigned char c) {
        return c >= 0x80;
    }

    void str_trim_end(std::string& string) {
        const auto pos = string.find_last_not_of(" \t\n\r");
        if (pos != std::string::npos) {
            string.erase(pos + 1);
        }
    }

    void str_replace_all(std::string& string, const std::string& old_value, const std::string& new_value) {
        if (string.empty()) {
            return;
        }
        KPL_ASSERT_THAT(!old_value.empty());
        size_t position = 0;
        while ((position = string.find(old_value, position)) != string.npos) {
            string.replace(position, old_value.size(), new_value);
            position += new_value.size();
        }
    }

    void str_initial_to_lower_case(std::string& string) {
        if (string.empty()) {
            return;
        }
        string[0] = std::tolower(string[0]);
    }

}
