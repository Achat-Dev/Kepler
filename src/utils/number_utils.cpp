// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include <cstdint>

namespace kepler {

    uint32_t get_digit_count(uint32_t number) {
        uint32_t result = 1;
        // Use integer division by 10 to "remove" the last digit until the value is smaller than 10, which means last digit reached
        while (number >= 10) {
            number /= 10;
            result += 1;
        }
        return result;
    }

}
