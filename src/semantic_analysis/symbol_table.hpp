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
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "string_pool.hpp"
#include "type_system/type.hpp"
#include <cstdint>
#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler {

    class SymbolTable {
    public:
        SymbolTable();
        std::expected<const Symbol*, SourceDiagnostic> create_variable(Type* type, StringId identifier_id, SourceLocation source_location);
        std::expected<const Symbol*, SourceDiagnostic> create_prototype(Type* type,
            StringId identifier_id,
            Prototype::LinkageType linkage_type,
            std::vector<Type*> parameter_types,
            SourceLocation identifier_source_location);
        const Symbol* lookup(StringId identifier_id) const;
        const Symbol* lookup_visible(StringId identifier_id) const;
        void open_scope(ScopeType type);
        void close_scope();

    private:
        std::vector<Symbol> symbols;
        std::vector<Scope> scopes;
        std::unordered_map<StringId, uint32_t> visible_symbols;

        std::expected<const Symbol*, SourceDiagnostic> create_symbol(Type* type,
            StringId identifier_id,
            SymbolData&& symbol_data,
            const std::string& error_identifier,
            SourceLocation source_location);
    };

}
