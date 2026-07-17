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
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler::semantic_analysis {

    class SymbolTable {
    public:
        SymbolTable();
        std::expected<const Symbol*, diagnostics::SourceDiagnostic> create_variable(type_system::DataTypeKind data_type, const std::string& identifier, const diagnostics::SourceLocation& source_location);
        std::expected<const Symbol*, diagnostics::SourceDiagnostic> create_prototype(type_system::DataTypeKind data_type, const std::string& identifier, ast::Prototype::LinkageType linkage_type, std::vector<type_system::DataTypeKind> parameter_data_types, const diagnostics::SourceLocation& source_location);
        const Symbol* lookup(const std::string& identifier) const;
        void open_scope(ScopeType type);
        void close_scope();

    private:
        std::vector<Symbol> symbols;
        std::vector<Scope> scopes;
        std::unordered_map<std::string, uint32_t> visible_symbols;

        std::expected<const Symbol*, diagnostics::SourceDiagnostic> create_symbol(type_system::DataTypeKind data_type, const std::string& identifier, SymbolData&& symbol_data, const std::string& error_identifier, const diagnostics::SourceLocation& source_location);
    };

}
