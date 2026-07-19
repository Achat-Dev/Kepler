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
#include <vector>

namespace kepler {

    struct ScopeId {
        uint32_t value = 0;

        constexpr bool is_valid() const { return value == std::numeric_limits<uint32_t>::max(); }
        static constexpr ScopeId invalid() { return ScopeId{std::numeric_limits<uint32_t>::max()}; }
    };

    enum class ScopeType {
        File,
        Function,
        Block,
    };

    struct Scope {
        ScopeId id;
        ScopeType type;
        ScopeId parent_id;
        std::vector<uint32_t> symbol_indices;

        Scope(ScopeType type, ScopeId parent_id)
            : id(id_creator++), type(type), parent_id(parent_id) {}

    private:
        inline static uint32_t id_creator = 0;
    };

}
