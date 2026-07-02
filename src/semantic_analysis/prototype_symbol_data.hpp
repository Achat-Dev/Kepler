// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/prototype.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include <vector>

namespace kepler::semantic_analysis {

    struct PrototypeSymbolData {
        std::vector<SymbolId> parameter_ids;
        ast::Prototype::LinkageType linkage_type;

        bool operator==(const PrototypeSymbolData& other) const = default;
        bool operator!=(const PrototypeSymbolData& other) const = default;
    };

}
