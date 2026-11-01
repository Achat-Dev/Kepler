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
#include <limits>

namespace kepler {

    // This needs to be in a separate file because of cyclic includes
    struct SymbolId {
        uint32_t value = std::numeric_limits<uint32_t>::max();

        bool operator==(const SymbolId& other) const = default;
        bool operator!=(const SymbolId& other) const = default;
        static constexpr SymbolId invalid() { return SymbolId{}; }
    };

}
