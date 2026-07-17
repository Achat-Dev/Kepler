// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "io/file_id.hpp"
#include <cstdint>

namespace kepler::diagnostics {

    struct SourceLocation {
        io::FileId file_id;
        uint32_t position;
        uint32_t size;

        SourceLocation(io::FileId file_id, uint32_t position, uint32_t size)
            : file_id(file_id), position(position), size(size) {}
    };

}
