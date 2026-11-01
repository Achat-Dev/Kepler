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
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <expected>
#include <vector>

namespace kepler {

    class SymbolTable {
    public:
        SymbolTable(TypeTable& type_table);
        ModuleId create_module(const IdentifierPath& module_path);
        // Symbols currently don't have a way to access their source location
        // However, the call site of these function *has* access to it, so we just return a normal Diagnostic instead of a SourceDiagnostic
        std::expected<ModuleId, Diagnostic> register_imported_module(ModuleId module_id, const IdentifierPath& imported_module_path);
        std::expected<SymbolId, Diagnostic> create_struct(ModuleId module_id, StringId identifier_id);
        std::expected<SymbolId, Diagnostic> create_variable(ModuleId module_id, TypeId type_id, StringId identifier_id);
        std::expected<SymbolId, Diagnostic> create_prototype(ModuleId module_id,
            TypeId return_type_id,
            StringId identifier_id,
            LinkageType linkage_type,
            std::vector<TypeId> parameter_type_ids,
            std::vector<StringId> parameter_type_identifier_ids,
            bool is_variadic,
            SymbolId type_symbol_id);

        Module* lookup(ModuleId module_id);
        Symbol* lookup(SymbolId symbol_id);
        // Note: Finding only searches global scopes after name resolution
        std::expected<std::vector<Symbol*>, Diagnostic> find_symbol_of_kind(ModuleId module_id, StringId identifier_id, SymbolKind symbol_kind);
        std::expected<std::vector<Symbol*>, Diagnostic> find_symbol_of_kind(ModuleId module_id,
            const IdentifierPath& module_path,
            StringId identifier_id,
            SymbolKind symbol_kind);
        void open_scope(ModuleId module_id, ScopeType type);
        void close_scope(ModuleId module_id);
        bool is_symbol_visible_in_module(ModuleId module_id, SymbolId symbol_id);
        Module* get_global_module();

    private:
        struct ScopeEntryVisitHandler {
            SymbolTable* symbol_table;
            bool is_imported_module;

            std::vector<Symbol*> operator()(SymbolId symbol_id) const;
            std::vector<Symbol*> operator()(const std::vector<SymbolId>& symbol_ids) const;
        };

        TypeTable& type_table;
        std::vector<Module> modules;
        std::vector<Symbol*> symbols;
        std::vector<Scope> scopes;
        ArenaAllocator symbol_allocator;

        std::expected<std::vector<Symbol*>, Diagnostic> find_symbol_of_kind(ModuleId module_id,
            StringId identifier_id,
            SymbolKind requested_symbol_kind,
            bool is_imported_module,
            bool search_imported_modules,
            bool is_recursive_search);
        Module* find_submodule(Module* parent_module, const IdentifierPath& module_path);
        Module* get_root_module();
    };

}
