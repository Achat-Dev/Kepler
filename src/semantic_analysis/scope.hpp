// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/symbol_id.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace kepler::semantic_analysis {

    using ScopeId = uint32_t;

    struct Scope {
        std::shared_ptr<Scope> parent;
        std::unordered_map<std::string, SymbolId> contained_identifiers;

        Scope(std::shared_ptr<Scope> parent) : parent(parent) {}
    };

}
