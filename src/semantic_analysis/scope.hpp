// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/string_pool.hpp"
#include <cstdint>
#include <unordered_map>

namespace kepler {

    struct ScopeId {
        uint32_t value = 0;

        bool operator==(const ScopeId& other) const = default;
        bool operator!=(const ScopeId& other) const = default;
        static constexpr ScopeId invalid() { return ScopeId{std::numeric_limits<uint32_t>::max()}; }
    };

    enum class ScopeType {
        File,
        Function,
        Block,
    };

    struct Scope {
        ScopeType type;
        ScopeId id;
        ScopeId parent_id;
        std::unordered_map<StringId, uint32_t> contained_symbols;
    };

}
