// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <variant>
#include <vector>

namespace kepler::semantic_analysis {

    using SymbolId = uint32_t;

    struct PrototypeSymbolData {
        std::vector<SymbolId> parameter_ids;

        bool operator==(const PrototypeSymbolData& other) const = default;
        bool operator!=(const PrototypeSymbolData& other) const = default;
    };

    struct Symbol {
        StringId identifier_id;
        type_system::DataTypeKind data_type;

        using SymbolData = std::variant<PrototypeSymbolData, std::monostate>;
        SymbolData data;
    };

}
