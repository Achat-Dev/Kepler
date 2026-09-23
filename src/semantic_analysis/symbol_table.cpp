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
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    SymbolTable::SymbolTable() {
        // Create the global module, which contains all other modules
        modules.push_back({.id = {.value = 0}, .full_identifier_id = StringPool::get().store("__global")});
    }

    ModuleId SymbolTable::create_module(const ModulePath& module_path) {
        KPL_ASSERT_THAT(!module_path.part_identifier_ids.empty());
        Module* module = get_global_module();
        // Walk the module path and create all missing modules along the way
        for (size_t i = 0; i < module_path.part_identifier_ids.size(); i++) {
            const StringId part_identifier_id = module_path.part_identifier_ids[i];
            const auto it = module->submodule_ids.find(part_identifier_id);
            if (it == module->submodule_ids.end()) {
                const ModuleId submodule_id = {.value = static_cast<uint32_t>(modules.size())};
                const auto [it, emplaced] = module->submodule_ids.emplace(part_identifier_id, submodule_id);
                KPL_ASSERT_THAT(emplaced);
                // Important: Do this last because otherwise the module pointer might be invalidated by the push
                const auto it_begin = module_path.part_identifier_ids.begin();
                // +1 because i is the index and the vector creation needs the size
                const ModulePath partial_module_path{.part_identifier_ids = std::vector<StringId>(it_begin, it_begin + i + 1)};
                modules.push_back({
                    .id = submodule_id,
                    .full_identifier_id = StringPool::get().store(get_full_module_identifier(partial_module_path)),
                });
                open_scope(submodule_id, ScopeType::File);
                module = &modules[submodule_id.value];
            } else {
                module = &modules[it->second.value];
            }
        }
        KPL_ASSERT_THAT(module->id != modules[0].id);
        return module->id;
    }

    std::expected<ModuleId, Diagnostic> SymbolTable::register_imported_module(ModuleId module_id, const ModulePath& imported_module_path) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!imported_module_path.part_identifier_ids.empty());
        Module* imported_module = find_module(get_global_module(), imported_module_path);
        if (imported_module == nullptr) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::UnknownModule,
                .message = std::format("Unknown imported module '{}'", get_full_module_identifier(imported_module_path)),
            });
        }

        Module& module = modules[module_id.value];
        if (&module == imported_module) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::InvalidImportOfSelf,
                .message = std::format("Can't import self. The import is discarded, but consider removing it."),
            });
        }

        const auto it_begin = module.imported_module_ids.begin();
        const auto it_end = module.imported_module_ids.end();
        if (std::find(it_begin, it_end, imported_module->id) != it_end) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::RedundantImport,
                .message = std::format("Module '{}' is already imported. Redundant imports are discarded, but consider removing them.",
                    StringPool::get().lookup(imported_module->full_identifier_id)),
            });
        }
        module.imported_module_ids.push_back(imported_module->id);
        return imported_module->id;
    }

    std::expected<SymbolId, Diagnostic> SymbolTable::create_variable(ModuleId module_id, Type* type, StringId identifier_id) {
        return create_symbol(module_id, type, identifier_id, std::monostate{}, "Variable");
    }

    // clang-format off
    std::expected<SymbolId, Diagnostic> SymbolTable::create_prototype(ModuleId module_id,
        Type* type,
        StringId identifier_id,
        LinkageType linkage_type,
        std::vector<Type*> parameter_types,
        bool is_variadic
    ) {
        // clang-format on
        return create_symbol(module_id,
            type,
            identifier_id,
            PrototypeSymbolData{.linkage_type = linkage_type, .is_variadic = is_variadic, .parameter_types = std::move(parameter_types)},
            "Prototype");
    }

    Symbol* SymbolTable::lookup(SymbolId symbol_id) {
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), symbol_id.value);
        return &symbols[symbol_id.value];
    }

    std::expected<Symbol*, Diagnostic> SymbolTable::find_symbol(ModuleId module_id, StringId identifier_id) {
        return find_symbol(module_id, identifier_id, false, true);
    }

    std::expected<Symbol*, Diagnostic> SymbolTable::find_symbol(ModuleId module_id, const ModulePath& module_path, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!module_path.part_identifier_ids.empty());
        std::vector<Module*> found_modules;
        Module* fully_qualified_module = find_module(get_global_module(), module_path);
        if (fully_qualified_module != nullptr) {
            found_modules.push_back(fully_qualified_module);
        }

        const Module& module = modules[module_id.value];
        for (ModuleId imported_module_id : module.imported_module_ids) {
            Module& imported_module = modules[imported_module_id.value];
            Module* found_module = find_module(&imported_module, module_path);
            if (found_module != nullptr) {
                found_modules.push_back(found_module);
            }
        }

        if (found_modules.empty()) {
            return nullptr;
        } else if (found_modules.size() == 1) {
            if (found_modules[0]->id == module_id) {
                return find_symbol(found_modules[0]->id, identifier_id, false, false);
            } else {
                return find_symbol(found_modules[0]->id, identifier_id, true, false);
            }
        } else {
            std::string message = std::format("Module path '{}' is a submodule of multiple imported modules (",
                get_full_module_identifier(module_path));
            for (size_t i = 0; i < found_modules.size(); i++) {
                message += '\'' + std::string(StringPool::get().lookup(found_modules[i]->full_identifier_id)) + '\'';
                if (i == found_modules.size() - 1) {
                    message += ')';
                } else {
                    message += ',';
                }
            }
            message += ". Must use the fully qualified module path in this case.";
            return std::unexpected(Diagnostic{.code = DiagnosticCode::AmbiguousModulePath, .message = std::move(message)});
        }
    }

    // clang-format off
    std::expected<Symbol*, Diagnostic> SymbolTable::find_symbol(ModuleId module_id,
        StringId identifier_id,
        bool is_imported_module,
        bool search_imported_modules)
    {
        // clang-format on
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
                if (is_imported_module) {
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

                std::vector<std::pair<ModuleId, Symbol*>> found_symbols;
                for (ModuleId imported_module_id : module.imported_module_ids) {
                    // If recursive imports are wanted, the second false here needs to be changed to true
                    const auto symbol = find_symbol(imported_module_id, identifier_id, true, false);
                    KPL_ASSERT_THAT(symbol.has_value()); // find only returns a diagnostic if imported symbols are searched
                    if (*symbol != nullptr) {
                        found_symbols.push_back({imported_module_id, *symbol});
                    }
                }

                if (found_symbols.empty()) {
                    return nullptr;
                } else if (found_symbols.size() == 1) {
                    return found_symbols[0].second;
                } else {
                    std::string message = std::format("Symbol '{}' found in multiple imported modules (", StringPool::get().lookup(identifier_id));
                    for (size_t i = 0; i < found_symbols.size(); i++) {
                        const Module& imported_module = modules[found_symbols[i].first.value];
                        message += '\'' + std::string(StringPool::get().lookup(imported_module.full_identifier_id)) + '\'';
                        if (i == found_symbols.size() - 1) {
                            message += ')';
                        } else {
                            message += ',';
                        }
                    }
                    const Module& imported_module = modules[found_symbols[0].first.value];
                    const std::string identifier = std::string(StringPool::get().lookup(identifier_id));
                    const std::string imported_module_identifier = std::string(StringPool::get().lookup(imported_module.full_identifier_id));
                    // clang-format off
                    message += ". Must use the fully qualified name in this case (e. g. '"
                        + std::move(imported_module_identifier)
                        + "::"
                        + std::move(identifier)
                        + "')";
                    // clang-format on
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
        // This check is theoretically not needed because module.current_scope_id is ScopeId::invalid() if no scope exists in the module
        // But do it like this because it's more explicit
        if (module.scope_ids.empty()) {
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

    // clang-format off
    std::expected<SymbolId, Diagnostic> SymbolTable::create_symbol(ModuleId module_id,
        Type* type,
        StringId identifier_id,
        SymbolData&& data,
        const std::string& error_identifier)
    {
        // clang-format on
        KPL_ASSERT_THAT(!scopes.empty());
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(!error_identifier.empty());

        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);

        const auto found_symbol = find_symbol(module_id, identifier_id, false, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const Symbol* existing_symbol = *found_symbol;
        SymbolId symbol_id_to_shadow;
        if (existing_symbol != nullptr) {
            if (existing_symbol->scope_id == module.current_scope_id) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol->identifier_id);
                return std::unexpected(Diagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists in the current scope", error_identifier, identifier),
                });
            }
            if (!existing_symbol->can_be_shadowed) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol->identifier_id);
                return std::unexpected(Diagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists and cannot be shadowed", error_identifier, identifier),
                });
            }
            symbol_id_to_shadow = existing_symbol->id;
        }

        Scope& scope = scopes[module.current_scope_id.value];
        bool can_be_shadowed = scope.type != ScopeType::Function && scope.type != ScopeType::Block;
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id);
        symbols.push_back({
            .id = symbol_id,
            .scope_id = scope.id,
            .type = type,
            .identifier_id = identifier_id,
            .can_be_shadowed = can_be_shadowed,
            .shadowed_symbol_id = symbol_id_to_shadow,
            .data = std::move(data),
        });
        return symbol_id;
    }

    Module* SymbolTable::find_module(Module* parent_module, const ModulePath& module_path) {
        KPL_ASSERT_NOT_NULLPTR(parent_module);
        KPL_ASSERT_THAT(!module_path.part_identifier_ids.empty());
        Module* result = parent_module;
        for (StringId part_identifier_id : module_path.part_identifier_ids) {
            const auto it = result->submodule_ids.find(part_identifier_id);
            if (it == result->submodule_ids.end()) {
                return nullptr;
            } else {
                result = &modules[it->second.value];
            }
        }
        if (result == parent_module) {
            return nullptr;
        }
        return result;
    }

    Module* SymbolTable::get_global_module() {
        return &modules.front();
    }
}
