// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "log.hpp"
#include <cstdlib>

namespace kepler {

    inline void emergency_exit() {
        const char h[] = "\u2500";
        const char v[] = "\u2502";
        const char tl[] = "\u250C";
        const char tr[] = "\u2510";
        const char bl[] = "\u2514";
        const char br[] = "\u2518";

        // clang-format off
        log(tl, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, "[ Emergency exit ]", h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, tr, "\n",
            v, "         Roses are red, violets are blue,        ", v, "\n",
            v, "   I reached some code that I never should do.   ", v, "\n",
            v, "       Now here I am, with no helping hand,      ", v, "\n",
            v, " And a crash and stack trace I don’t understand. ", v, "\n",
            bl, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, h, br);
        // clang-format on
        std::abort();
    }

}
