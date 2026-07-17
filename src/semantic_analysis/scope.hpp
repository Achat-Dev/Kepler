// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstdint>
#include <vector>

namespace kepler::semantic_analysis {

    struct ScopeID {
        uint32_t value;
    };

    enum class ScopeType {
        File,
        Function,
        Block,
    };

    struct Scope {
        ScopeID id;
        ScopeType type;
        ScopeID parent_id;
        std::vector<uint32_t> symbol_indices;

        Scope(ScopeType type, ScopeID parent_id)
            : id(id_creator++), type(type), parent_id(parent_id) {}

    private:
        inline static uint32_t id_creator = 0;
    };

}
