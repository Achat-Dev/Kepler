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
#include "type_system/type.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <deque>
#include <expected>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    class SymbolTable {
    public:
        SymbolTable();
        std::expected<Symbol*, SourceDiagnostic> create_variable(Type* type, StringId identifier_id, SourceLocation source_location);
        std::expected<Symbol*, SourceDiagnostic> create_prototype(Type* type,
            StringId identifier_id,
            Prototype::LinkageType linkage_type,
            std::vector<Type*> parameter_types,
            bool is_variadic,
            SourceLocation identifier_source_location);

        // TODO (check): Maybe create a method to disable lookup after name resolution
        // Note: This method should only be used during name resolution.
        Symbol* lookup(StringId identifier_id);
        void open_scope(ScopeType type);
        void close_scope();

    private:
        // TODO (check): Look at the usage of std::deque again, that was a quick fix but maybe not the best choice
        std::deque<Symbol> symbols;
        std::vector<Scope> scopes;
        Scope* current_scope;

        std::expected<Symbol*, SourceDiagnostic> create_symbol(Type* type,
            StringId identifier_id,
            SymbolData&& symbol_data,
            const std::string& error_identifier,
            SourceLocation source_location);
        std::pair<Symbol*, uint32_t> lookup_with_index(StringId identifier_id, ScopeId scope_id);
    };

}
