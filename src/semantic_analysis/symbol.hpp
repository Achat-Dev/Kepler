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
#include "type_system/data_type_kind.hpp"
#include <string>
#include <variant>

namespace kepler::semantic_analysis {

    using SymbolData = std::variant<PrototypeSymbolData, std::monostate>;

    struct Symbol {
        std::string identifier_id;
        type_system::DataTypeKind data_type;
        SymbolData data;
    };

}
