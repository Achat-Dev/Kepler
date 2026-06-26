// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "error_code.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <expected>
#include <memory>
#include <vector>

namespace kepler::semantic_analysis {

    class SymbolTable {
    public:
        std::expected<SymbolId, ErrorCode> create_variable(StringId identifier_id, type_system::DataTypeKind type);
        std::expected<SymbolId, ErrorCode> create_prototype(StringId identifier_id, type_system::DataTypeKind type, PrototypeSymbolData data);
        const Symbol& lookup(SymbolId id) const;

        void open_scope();
        void close_scope();

        static SymbolTable& get();

    private:
        std::vector<Symbol> symbols;
        std::vector<std::shared_ptr<Scope>> scopes;

        SymbolTable();
    };

}
