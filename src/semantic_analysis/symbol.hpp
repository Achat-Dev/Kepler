// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace kepler::semantic_analysis {

    using SymbolData = std::variant<PrototypeSymbolData, std::monostate>;

    struct Symbol {
        ScopeID scope_id;
        type_system::DataTypeKind data_type;
        std::string identifier;
        bool can_be_shadowed;
        uint32_t shadowed_symbol_index;
        SymbolData data;

        Symbol(ScopeID scope_id,
            type_system::DataTypeKind data_type,
            const std::string& identifier,
            bool can_be_shadowed,
            uint32_t shadowed_symbol_index,
            SymbolData data)
            : scope_id(scope_id),
              data_type(data_type),
              identifier(identifier),
              can_be_shadowed(can_be_shadowed),
              shadowed_symbol_index(shadowed_symbol_index),
              data(std::move(data)) {}
    };

}
