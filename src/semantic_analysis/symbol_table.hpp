// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/data_type_kind.hpp"
#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace kepler::semantic_analysis {

    class SymbolTable {
    public:
        SymbolTable(const SymbolTable& string_table) = delete;
        void operator=(const SymbolTable& string_table) = delete;

        std::expected<SymbolId, diagnostics::Diagnostic> create_variable(const std::string& identifier, type_system::DataTypeKind data_type);
        std::expected<SymbolId, diagnostics::Diagnostic> create_prototype(const std::string& identifier, type_system::DataTypeKind data_type, PrototypeSymbolData data);
        const Symbol& lookup(SymbolId id) const;

        void open_scope();
        void close_scope();
        bool does_name_exist_in_scope_stack(const std::string& identifier) const;

        static SymbolTable& get();

    private:
        std::vector<Symbol> symbols;
        std::vector<std::shared_ptr<Scope>> scopes;

        SymbolTable();
    };

}
