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
#include "type_system/type_table.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
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

    SymbolTable::SymbolTable()
        : symbol_allocator(kibibyte_size * 32, "symbols") {
        // Create the global module, which contains all other modules
        modules.push_back({.id = {.value = 0}, .full_identifier_id = StringPool::get().store("__global")});
    }

    std::vector<Symbol*> SymbolTable::ScopeEntryVisitHandler::operator()(SymbolId symbol_id) const {
        KPL_ASSERT_NOT_NULLPTR(symbol_table);
        KPL_ASSERT_THAT(symbol_id.value < symbol_table->symbols.size(),
            "Symbol count: {}, received id: {}",
            symbol_table->symbols.size(),
            symbol_id.value);
        Symbol* symbol = symbol_table->symbols[symbol_id.value];
        if (symbol->get_symbol_kind() == requested_symbol_kind || requested_symbol_kind == SymbolKind::All) {
            if (is_imported_module) {
                if (symbol->get_symbol_kind() == SymbolKind::Prototype) {
                    // TODO (improvement): Maybe create a special diagnostic if a fitting symbol is found but it is not exported
                    // (That diagnostic shouldn't be displayed immediately, but only when no fitting symbol is found in all imported modules)
                    if (static_cast<PrototypeSymbol*>(symbol)->linkage_type == LinkageType::Export) {
                        return std::vector<Symbol*>{symbol};
                    }
                }
            } else {
                return std::vector<Symbol*>{symbol};
            }
        }
        return std::vector<Symbol*>{};
    }

    std::vector<Symbol*> SymbolTable::ScopeEntryVisitHandler::operator()(const std::vector<SymbolId>& symbol_ids) const {
        KPL_ASSERT_NOT_NULLPTR(symbol_table);
        // Only prototypes can be overloaded, so if a scope entry contains a list of overloaded prototypes, we only care if the
        if (requested_symbol_kind != SymbolKind::Prototype && requested_symbol_kind != SymbolKind::All) {
            return std::vector<Symbol*>{};
        }
        std::vector<Symbol*> result;
        result.reserve(symbol_ids.size());
        for (const SymbolId symbol_id : symbol_ids) {
            KPL_ASSERT_THAT(symbol_id.value < symbol_table->symbols.size(),
                "Symbol count: {}, received id: {}",
                symbol_table->symbols.size(),
                symbol_id.value);
            Symbol* symbol = symbol_table->symbols[symbol_id.value];
            KPL_ASSERT_THAT(symbol->get_symbol_kind() == SymbolKind::Prototype);
            // TODO (improvement): Maybe create a special diagnostic if a fitting symbol is found but it is not exported
            // (That diagnostic shouldn't be displayed immediately, but only when no fitting symbol is found in all imported modules)
            if (is_imported_module) {
                if (static_cast<PrototypeSymbol*>(symbol)->linkage_type == LinkageType::Export) {
                    result.push_back(symbol);
                }
            } else {
                result.push_back(symbol);
            }
        }
        return result;
    }

    ModuleId SymbolTable::create_module(const IdentifierPath& module_path) {
        KPL_ASSERT_THAT(!module_path.identifier_parts.empty());
        Module* module = get_global_module();
        // Walk the module path and create all missing modules along the way
        for (size_t i = 0; i < module_path.identifier_parts.size(); i++) {
            const StringId part_identifier_id = module_path.identifier_parts[i].identifier_id;
            KPL_ASSERT_THAT(part_identifier_id != StringId::invalid());
            const auto it = module->submodule_ids.find(part_identifier_id);
            if (it == module->submodule_ids.end()) {
                const ModuleId submodule_id = {.value = static_cast<uint32_t>(modules.size())};
                const auto [it, emplaced] = module->submodule_ids.emplace(part_identifier_id, submodule_id);
                KPL_ASSERT_THAT(emplaced);
                // Important: Do this last because otherwise the module pointer might be invalidated by the push
                const auto it_begin = module_path.identifier_parts.begin();
                // +1 because i is the index and the vector creation needs the size
                const IdentifierPath submodule_path{
                    .identifier_parts = std::vector<IdentifierPathPart>(it_begin, it_begin + i + 1),
                    .separator_id = module_path.separator_id,
                };
                modules.push_back({
                    .id = submodule_id,
                    .full_identifier_id = StringPool::get().store(get_full_identifier_from_path(submodule_path)),
                });
                open_scope(submodule_id, ScopeType::File);
                module = &modules[submodule_id.value];
            } else {
                module = &modules[it->second.value];
            }
        }
        KPL_ASSERT_THAT(module != get_global_module());
        return module->id;
    }

    std::expected<ModuleId, Diagnostic> SymbolTable::register_imported_module(ModuleId module_id, const IdentifierPath& imported_module_path) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!imported_module_path.identifier_parts.empty());
        Module* imported_module = find_submodule(get_global_module(), imported_module_path);
        if (imported_module == nullptr) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::UnknownModule,
                .message = std::format("Unknown imported module '{}'", get_full_identifier_from_path(imported_module_path)),
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

    std::expected<SymbolId, Diagnostic> SymbolTable::create_struct(ModuleId module_id, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        KPL_ASSERT_THAT(module.current_scope_id == module.scope_ids.front()); // Structs can only be defined on the top level
        // Structs are created first, so this can only return other structs
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            // KPL_ASSERT_THAT(existing_symbols.size() == 1);
            // KPL_ASSERT_NOT_NULLPTR(existing_symbols[0]);
            // KPL_ASSERT_THAT(existing_symbols[0]->scope_id == module.current_scope_id);
            // KPL_ASSERT_THAT(existing_symbols[0]->get_symbol_kind() == SymbolKind::Type);
            const std::string_view identifier = StringPool::get().lookup(identifier_id);
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::SymbolAlreadyExists,
                .message = std::format("Struct with name '{}' already exists in module '{}'",
                    identifier,
                    StringPool::get().lookup(module.full_identifier_id)),
            });
        }

        Scope& scope = scopes[module.current_scope_id.value];
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id);
        Symbol* symbol = symbol_allocator.allocate<TypeSymbol>(symbol_id, scope.id, identifier_id);
        symbols.push_back(symbol);
        return symbol_id;
    }

    std::expected<SymbolId, Diagnostic> SymbolTable::create_variable(ModuleId module_id, TypeId type_id, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            const std::string_view identifier = StringPool::get().lookup(identifier_id);
            if (existing_symbols.size() == 1) {
                if (existing_symbols[0]->scope_id == module.current_scope_id) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::SymbolAlreadyExists,
                        .message = std::format("Symbol with name '{}' already exists in the current scope", identifier),
                    });
                }
            }
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::SymbolAlreadyExists,
                .message = std::format("Symbol with name '{}' already exists and cannot be shadowed", identifier),
            });
        }

        Scope& scope = scopes[module.current_scope_id.value];
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id);
        Symbol* symbol = symbol_allocator.allocate<VariableSymbol>(symbol_id, scope.id, type_id, identifier_id);
        symbols.push_back(symbol);
        return symbol_id;
    }

    // clang-format off
    std::expected<SymbolId, Diagnostic> SymbolTable::create_prototype(ModuleId module_id,
        TypeId type_id,
        StringId identifier_id,
        LinkageType linkage_type,
        std::vector<TypeId> parameter_type_ids,
        std::vector<StringId> parameter_type_identifier_ids,
        bool is_variadic,
        TypeTable& type_table)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(parameter_type_ids.size() == parameter_type_identifier_ids.size());
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        KPL_ASSERT_THAT(module.current_scope_id == module.scope_ids.front()); // Prototypes can only be defined on the top level
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            const std::string_view identifier = StringPool::get().lookup(identifier_id);
            const std::string_view module_identifier = StringPool::get().lookup(module.full_identifier_id);
            for (const Symbol* existing_symbol : existing_symbols) {
                if (existing_symbol->get_symbol_kind() != SymbolKind::Prototype) {
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::SymbolAlreadyExists,
                        .message = std::format("Symbol with name '{}' already exists in module '{}'", identifier, module_identifier),
                    });
                }
                const PrototypeSymbol* existing_prototype_symbol = static_cast<const PrototypeSymbol*>(existing_symbol);
                if (existing_prototype_symbol->parameter_type_ids == parameter_type_ids && existing_prototype_symbol->is_variadic == is_variadic) {
                    const std::string message = std::format("Prototype with signature '{}' already exists in module '{}'",
                        get_prototype_symbol_signature(existing_prototype_symbol, type_table),
                        module_identifier);
                    return std::unexpected(Diagnostic{.code = DiagnosticCode::SymbolAlreadyExists, .message = std::move(message)});
                }
            }
        }

        Scope& scope = scopes[module.current_scope_id.value];
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        auto it = scope.contained_symbols.find(identifier_id);
        if (it == scope.contained_symbols.end()) {
            scope.contained_symbols.emplace(identifier_id, std::vector<SymbolId>{symbol_id});
        } else {
            KPL_ASSERT_THAT(std::holds_alternative<std::vector<SymbolId>>(it->second));
            std::get<std::vector<SymbolId>>(it->second).push_back(symbol_id);
        }
        PrototypeSymbol* symbol = symbol_allocator.allocate<PrototypeSymbol>(symbol_id,
            scope.id,
            type_id,
            identifier_id,
            linkage_type,
            std::move(parameter_type_ids),
            is_variadic);
        // symbol->signature_identifier_id = StringPool::get().store(get_prototype_symbol_signature(symbol));
        symbols.push_back(symbol);
        return symbol_id;
    }

    Symbol* SymbolTable::lookup(SymbolId symbol_id) {
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), symbol_id.value);
        return symbols[symbol_id.value];
    }

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol(ModuleId module_id, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, true);
    }

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol_of_kind(ModuleId module_id, StringId identifier_id, SymbolKind symbol_kind) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return find_symbol_of_kind(module_id, identifier_id, symbol_kind, false, true);
    }

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol(ModuleId module_id, const IdentifierPath& module_path, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!module_path.identifier_parts.empty());
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        std::vector<Module*> found_modules;
        Module* fully_qualified_module = find_submodule(get_global_module(), module_path);
        if (fully_qualified_module != nullptr) {
            found_modules.push_back(fully_qualified_module);
        }

        Module& module = modules[module_id.value];
        // Check if submodule of own module
        Module* found_module = find_submodule(&module, module_path);
        if (found_module != nullptr) {
            found_modules.push_back(found_module);
        }
        // Check if submodule of imported module
        for (ModuleId imported_module_id : module.imported_module_ids) {
            KPL_ASSERT_THAT(imported_module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), imported_module_id.value);
            Module& imported_module = modules[imported_module_id.value];
            Module* found_module = find_submodule(&imported_module, module_path);
            if (found_module != nullptr) {
                found_modules.push_back(found_module);
            }
        }

        if (found_modules.empty()) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::UnknownModule,
                .message = std::format("Unknown module '{}'", get_full_identifier_from_path(module_path)),
            });
        } else if (found_modules.size() == 1) {
            if (found_modules[0]->id == module_id) {
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, SymbolKind::All, false, false);
            } else {
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, SymbolKind::All, true, false);
            }
        } else {
            // TODO (improvement): Right now there will always be an error as soon as an ambiguous module path is found,
            // even if the wanted symbol only exists in one module.
            // It would be nice to only report a diagnostic if the symbol actually exists in multiple modules
            std::string message = std::format("Module identifier '{}' refers to multiple modules:\n",
                get_full_identifier_from_path(module_path));
            for (size_t i = 0; i < found_modules.size(); i++) {
                message += std::format("{}{}) {}", log::sub_line, i + 1, std::string(StringPool::get().lookup(found_modules[i]->full_identifier_id)));
                if (i < found_modules.size() - 1) {
                    message += '\n';
                }
            }
            return std::unexpected(Diagnostic{.code = DiagnosticCode::AmbiguousModulePath, .message = std::move(message)});
        }
    }

    // clang-format off
    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol_of_kind(ModuleId module_id,
        StringId identifier_id,
        SymbolKind requested_symbol_kind,
        bool is_imported_module,
        bool search_imported_modules)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const Module& module = modules[module_id.value];

        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        Scope* scope = &scopes[module.current_scope_id.value];
        std::vector<Symbol*> result;
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto it = scope->contained_symbols.find(identifier_id);
            if (it != scope->contained_symbols.end()) {
                std::vector<Symbol*> symbols = std::visit(
                    ScopeEntryVisitHandler{
                        .symbol_table = this,
                        .requested_symbol_kind = requested_symbol_kind,
                        .is_imported_module = is_imported_module,
                    },
                    it->second);
                if (!symbols.empty()) {
                    result.insert(result.end(), symbols.begin(), symbols.end());
                }
            }

            if (scope->parent_id == ScopeId::invalid()) {
                if (!search_imported_modules) {
                    return result;
                }

                std::unordered_map<ModuleId, std::vector<Symbol*>> found_imported_symbols;
                for (ModuleId imported_module_id : module.imported_module_ids) {
                    KPL_ASSERT_THAT(imported_module_id.value < modules.size(),
                        "Module count: {}, received id: {}",
                        modules.size(),
                        imported_module_id.value);
                    // If recursive imports are wanted, the second false here needs to be changed to true
                    const auto imported_symbols = find_symbol_of_kind(imported_module_id, identifier_id, requested_symbol_kind, true, false);
                    KPL_ASSERT_THAT(imported_symbols.has_value()); // find only returns a diagnostic if imported symbols are searched
                    // Every found symbol is of the same kind, so only checking if the first symbol kind is correct is enough
                    // This is the case because every imported symbol only searches itself, which means it can either find
                    // - Exactly one variable with the given identifier
                    // - Exactly one type with the given identifier
                    // - Multiple prototypes with the given identifier
                    // const SymbolKind symbol_kind = imported_symbols.value().front()->get_symbol_kind();
                    // const bool is_valid_symbol_kind = symbol_kind == requested_symbol_kind || requested_symbol_kind == SymbolKind::All;
                    if (!imported_symbols.value().empty() /* && is_valid_symbol_kind*/) {
                        found_imported_symbols.emplace(imported_module_id, imported_symbols.value());
                    }
                }

                // Only found symbols in own module -> return them
                if (found_imported_symbols.empty()) {
                    return result;
                }
                // Symbols found in own and/or multiple imported modules -> maybe ambiguous symbols
                if (!result.empty() || found_imported_symbols.size() != 1) {
                    std::string message = std::format("Symbol '{}' found in multiple {}modules (",
                        StringPool::get().lookup(identifier_id),
                        result.empty() ? "" : "imported ");
                    if (!result.empty()) {
                        message += std::format("'{}(self)',", StringPool::get().lookup(module.full_identifier_id));
                    }
                    size_t i = 0;
                    for (const auto& [module_id, found_symbols] : found_imported_symbols) {
                        const Module& imported_module = modules[module_id.value];
                        message += '\'' + std::string(StringPool::get().lookup(imported_module.full_identifier_id)) + '\'';
                        if (i == found_imported_symbols.size() - 1) {
                            message += ')';
                        } else {
                            message += ", ";
                        }
                        i++;
                    }
                    const Module& imported_module = modules[found_imported_symbols.begin()->first.value];
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
                // Symbols only found in exactly one imported module -> return them
                return std::move(found_imported_symbols.begin()->second);
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
            scopes.emplace_back(type, scope_id, ScopeId::invalid(), std::unordered_map<StringId, ScopeEntry>{});
        } else {
            scopes.emplace_back(type, scope_id, module.current_scope_id, std::unordered_map<StringId, ScopeEntry>{});
        }
        module.scope_ids.push_back(scope_id);
        module.current_scope_id = scope_id;
    }

    void SymbolTable::close_scope(ModuleId module_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        const Scope& scope = scopes[module.current_scope_id.value];
        KPL_ASSERT_THAT(scope.parent_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), scope.parent_id.value);
        module.current_scope_id = scope.parent_id;
    }

    // clang-format off
    /*std::expected<SymbolId, Diagnostic> SymbolTable::create_symbol(ModuleId module_id,
        TypeId type_id,
        StringId identifier_id,
        SymbolKind symbol_kind)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(symbol_kind != SymbolKind::All);
        // type_id can be invalid because structs are created with an invalid type id, that's why there is no assert for that

        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);

        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const Symbol* existing_symbol = found_symbol.value();
        if (existing_symbol != nullptr) {
            const std::string_view identifier = StringPool::get().lookup(existing_symbol->identifier_id);
            if (existing_symbol->scope_id == module.current_scope_id) {
                return std::unexpected(Diagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("Symbol with name '{}' already exists in the current scope", identifier),
                });
            }
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::SymbolAlreadyExists,
                .message = std::format("Symbol with name '{}' already exists and cannot be shadowed", identifier),
            });
        }

        Scope& scope = scopes[module.current_scope_id.value];
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id);
        symbols.push_back({
            .id = symbol_id,
            .scope_id = scope.id,
            .type_id = type_id,
            .identifier_id = identifier_id,
            .symbol_kind = symbol_kind,
            .data = std::move(data),
        });
        return symbol_id;
    }*/

    Module* SymbolTable::find_submodule(Module* parent_module, const IdentifierPath& module_path) {
        KPL_ASSERT_NOT_NULLPTR(parent_module);
        KPL_ASSERT_THAT(!module_path.identifier_parts.empty());
        Module* result = parent_module;
        for (const IdentifierPathPart& identifier_part : module_path.identifier_parts) {
            KPL_ASSERT_THAT(identifier_part.identifier_id != StringId::invalid());
            const auto it = result->submodule_ids.find(identifier_part.identifier_id);
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
        KPL_ASSERT_THAT(!modules.empty());
        return &modules.front();
    }
}
