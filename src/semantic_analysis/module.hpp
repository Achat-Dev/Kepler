// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/scope.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace kepler {

    struct ModuleId {
        uint32_t value = std::numeric_limits<uint32_t>::max();

        bool operator==(const ModuleId& other) const = default;
        bool operator!=(const ModuleId& other) const = default;
        static constexpr ModuleId invalid() { return ModuleId{}; }
    };

    struct ModuleDefinition {
        ModuleId id;
        StringId full_identifier_id;
        std::vector<StringId> part_identifier_ids;
    };

    struct Module {
        ModuleDefinition definition;
        std::vector<ScopeId> scope_ids;
        ScopeId current_scope_id;
    };

}
