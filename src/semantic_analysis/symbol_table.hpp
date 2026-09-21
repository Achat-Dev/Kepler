// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/string_pool.hpp"
#include <expected>
#include <string>
#include <vector>

namespace kepler {

    class SymbolTable {
    public:
        SymbolTable();
        ModuleId create_module(const ModulePath& module_path);
        std::expected<ModuleId, Diagnostic> register_imported_module(ModuleId module_id, const ModulePath& imported_module_path);
        // TODO (improvement): Just return a diagnostic, because the source location is available at the call side
        std::expected<SymbolId, SourceDiagnostic> create_variable(ModuleId module_id, Type* type, StringId identifier_id, SourceLocation source_location);
        std::expected<SymbolId, SourceDiagnostic> create_prototype(ModuleId module_id,
            Type* type,
            StringId identifier_id,
            LinkageType linkage_type,
            std::vector<Type*> parameter_types,
            bool is_variadic,
            SourceLocation identifier_source_location);

        Symbol* lookup(SymbolId symbol_id);
        // TODO (check): Maybe create a method to disable finding after name resolution
        // Note: This method should only be used during name resolution.
        std::expected<Symbol*, Diagnostic> find_symbol(ModuleId module_id, StringId identifier_id);
        std::expected<Symbol*, Diagnostic> find_symbol(ModuleId module_id, const ModulePath& module_path, StringId identifier_id);
        void open_scope(ModuleId module_id, ScopeType type);
        void close_scope(ModuleId module_id);

    private:
        std::vector<Module> modules;
        std::vector<Symbol> symbols;
        std::vector<Scope> scopes;

        std::expected<SymbolId, SourceDiagnostic> create_symbol(ModuleId module_id,
            Type* type,
            StringId identifier_id,
            SymbolData&& symbol_data,
            const std::string& error_identifier,
            SourceLocation source_location);
        std::expected<Symbol*, Diagnostic> find_symbol(ModuleId module_id, StringId identifier_id, bool search_imported_modules);
        Module* find_module(Module* parent_module, const ModulePath& module_path);
        Module* get_global_module();
    };

}
