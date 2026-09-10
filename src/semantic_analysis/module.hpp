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
#include <vector>

namespace kepler {

    struct ModuleId {
        uint32_t value;
    };

    struct Module {
        std::vector<StringId> identifier_ids;
        std::vector<ScopeId> scope_ids;
        ScopeId current_scope_id;
    };

}
