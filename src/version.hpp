// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstdint>

namespace kepler {

    struct Version {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
    };

    inline constexpr Version version{.major = 0, .minor = 1, .patch = 0};

}
