// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "ast/ast_node.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define INVALID_SYMBOL_INDEX std::numeric_limits<uint32_t>::max()

namespace kepler {

    ModuleId SymbolTable::create_module(StringId identifier_id) {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        ModuleId module_id = get_module_id_by_identifier(identifier_id);
        if (module_id == ModuleId::invalid()) {
            module_id.value = static_cast<uint32_t>(modules.size());
        }
        modules.push_back({.id = module_id, .identifier_id = identifier_id});
        open_scope(module_id, ScopeType::File);
        return module_id;
    }

    void SymbolTable::register_imported_modules(ModuleId module_id, std::vector<StringId> imported_module_identifier_ids) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        Module& module = modules[module_id.value];
        for (StringId imported_module_identifier_id : imported_module_identifier_ids) {
            ModuleId imported_module_id = get_module_id_by_identifier(imported_module_identifier_id);
            if (imported_module_id == ModuleId::invalid()) {
                // We are trying to import a module that doesn't explicitely exist, but a submodule exists
                // e. g. module foo::bar ... import foo <- foo was never explicitely defined
                // So we create that module as an empty module
                imported_module_id = create_module(imported_module_identifier_id);
            }
            module.imported_module_ids.push_back(imported_module_id);
        }
    }

    // clang-format off
    std::expected<SymbolId, SourceDiagnostic> SymbolTable::create_variable(ModuleId module_id,
        Type* type,
        StringId identifier_id,
        SourceLocation source_location)
    {
        // clang-format on
        return create_symbol(module_id, type, identifier_id, std::monostate{}, "Variable", source_location);
    }

    // clang-format off
    std::expected<SymbolId, SourceDiagnostic> SymbolTable::create_prototype(ModuleId module_id,
        Type* type,
        StringId identifier_id,
        LinkageType linkage_type,
        std::vector<Type*> parameter_types,
        bool is_variadic,
        SourceLocation identifier_source_location
    ) {
        // clang-format on
        return create_symbol(module_id,
            type,
            identifier_id,
            PrototypeSymbolData{.linkage_type = linkage_type, .is_variadic = is_variadic, .parameter_types = std::move(parameter_types)},
            "Prototype",
            identifier_source_location);
    }

    Symbol* SymbolTable::lookup(SymbolId symbol_id) {
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), symbol_id.value);
        return &symbols[symbol_id.value];
    }

    std::expected<Symbol*, Diagnostic> SymbolTable::find(ModuleId module_id, StringId identifier_id) {
        return find(module_id, identifier_id, true);
    }

    std::expected<Symbol*, Diagnostic> SymbolTable::find(ModuleId module_id, StringId identifier_id, bool search_imported_modules) {
        KPL_ASSERT_THAT(!scopes.empty());
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        const Module& module = modules[module_id.value];

        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        Scope* scope = &scopes[module.current_scope_id.value];
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto it = scope->contained_symbols.find(identifier_id);
            if (it != scope->contained_symbols.end()) {
                KPL_ASSERT_THAT(it->second.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), it->second.value);
                Symbol* symbol = &symbols[it->second.value];
                // Currently, if search_imported_modules is false, we are in an imported module
                // That means that the symbol has to be exported in order to be found
                if (search_imported_modules == false) {
                    if (std::holds_alternative<PrototypeSymbolData>(symbol->data)) {
                        // TODO (improvement): Maybe create a special diagnostic if a fitting symbol is found but it is not exported
                        // (That diagnostic shouldn't be displayed immediately, but only when no fitting symbol is found in all imported modules)
                        if (std::get<PrototypeSymbolData>(symbol->data).linkage_type == LinkageType::Export) {
                            return symbol;
                        }
                    }
                } else {
                    return symbol;
                }
            }

            if (scope->parent_id == ScopeId::invalid()) {
                if (!search_imported_modules) {
                    return nullptr;
                }

                std::vector<Symbol*> found_symbols;
                for (ModuleId imported_module_id : module.imported_module_ids) {
                    const auto symbol = find(imported_module_id, identifier_id, false); // Use false here so that there are no recursive imports
                    KPL_ASSERT_THAT(symbol.has_value());                                // find only returns a diagnostic if imported symbols are searched
                    if (*symbol != nullptr) {
                        found_symbols.push_back(*symbol);
                    }
                }

                if (found_symbols.empty()) {
                    return nullptr;
                } else if (found_symbols.size() == 1) {
                    return found_symbols[0];
                } else {
                    std::string message = std::format("Symbol '{}' found in multiple imported modules (", StringPool::get().lookup(identifier_id));
                    for (size_t i = 0; found_symbols.size(); i++) {
                        message += std::format("'{}'", StringPool::get().lookup(found_symbols[i]->identifier_id));
                        if (i == found_symbols.size() - 1) {
                            message += ')';
                        } else {
                            message += ',';
                        }
                    }
                    // Symbols currently don't have a way to access their source location
                    // However, the call site of this function *has* access to it, so we just return a normal Diagnostic instead of a SourceDiagnostic
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::AmbiguousSymbolImport, .message = std::move(message)});
                }
            }
            KPL_ASSERT_THAT(scope->parent_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), scope->parent_id.value);
            scope = &scopes[scope->parent_id.value];
        }
    }

    void SymbolTable::open_scope(ModuleId module_id, ScopeType type) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        const ScopeId scope_id = {.value = static_cast<uint32_t>(scopes.size())};
        Module& module = modules[module_id.value];
        if (scopes.empty()) {
            scopes.emplace_back(type, scope_id, ScopeId::invalid(), std::unordered_map<StringId, SymbolId>{});
        } else {
            scopes.emplace_back(type, scope_id, module.current_scope_id, std::unordered_map<StringId, SymbolId>{});
        }
        module.scope_ids.push_back(scope_id);
        module.current_scope_id = scope_id;
    }

    void SymbolTable::close_scope(ModuleId module_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!scopes.empty());
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        const Scope& scope = scopes[module.current_scope_id.value];
        KPL_ASSERT_THAT(scope.parent_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), scope.parent_id.value);
        module.current_scope_id = scope.parent_id;
    }

    ModuleId SymbolTable::get_module_id_by_identifier(StringId identifier_id) const {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        // TODO (improvement): A linear search is maybe not the most performant implemenation for this
        for (size_t i = 0; i < modules.size(); i++) {
            if (modules[i].identifier_id == identifier_id) {
                return ModuleId{.value = static_cast<uint32_t>(i)};
            }
        }
        return ModuleId::invalid();
    }

    // clang-format off
    std::expected<SymbolId, SourceDiagnostic> SymbolTable::create_symbol(ModuleId module_id,
        Type* type,
        StringId identifier_id,
        SymbolData&& data,
        const std::string& error_identifier,
        SourceLocation source_location
    ) {
        // clang-format on
        KPL_ASSERT_THAT(!scopes.empty());
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(!error_identifier.empty());

        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);

        const auto found_symbol = find(module_id, identifier_id, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const Symbol* existing_symbol = *found_symbol;
        uint32_t symbol_index_to_shadow = INVALID_SYMBOL_INDEX;
        if (existing_symbol != nullptr) {
            if (existing_symbol->scope_id == module.current_scope_id) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol->identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists in the current scope", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            if (!existing_symbol->can_be_shadowed) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol->identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists and cannot be shadowed", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            symbol_index_to_shadow = existing_symbol->id.value;
        }

        Scope& scope = scopes[module.current_scope_id.value];
        bool can_be_shadowed = scope.type != ScopeType::Function && scope.type != ScopeType::Block;
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id.value);
        symbols.push_back({
            .id = symbol_id,
            .scope_id = scope.id,
            .type = type,
            .identifier_id = identifier_id,
            .can_be_shadowed = can_be_shadowed,
            .shadowed_symbol_index = symbol_index_to_shadow,
            .data = std::move(data),
        });
        return symbol_id;
    }

}
