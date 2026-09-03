// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "io/file.hpp"
#include <cstdint>

namespace kepler {

    // TODO (improvement): Some diagnostics don't point at the best locations
    // Most of the time the next avaiable source location is used, but that could be after a newline,
    // which can be a bit confusing
    struct SourceLocation {
        FileId file_id;
        uint32_t position = 0;
        uint32_t size = 0;
    };

}
