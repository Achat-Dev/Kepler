// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/prototype.hpp"
#include "semantic_analysis/scope.hpp"
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <variant>
#include <vector>

namespace kepler {

    struct PrototypeSymbolData {
        Prototype::LinkageType linkage_type;
        std::vector<DataTypeKind> parameter_data_types;
    };

    using SymbolData = std::variant<std::monostate, PrototypeSymbolData>;

    struct Symbol {
        ScopeId scope_id;
        DataTypeKind data_type;
        StringId identifier_id;
        bool can_be_shadowed = false;
        uint32_t shadowed_symbol_index = 0;
        SymbolData data = std::monostate{};
    };

}
