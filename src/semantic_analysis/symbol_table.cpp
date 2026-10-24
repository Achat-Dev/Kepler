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
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/log.hpp"
#include "utils/number_utils.hpp"
#include "utils/string_pool.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    SymbolTable::SymbolTable(TypeTable& type_table)
        : type_table(type_table), symbol_allocator(kibibyte_size * 32, "symbols") {
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
        if (is_imported_module) {
            // TODO (improvement): Maybe create a special diagnostic if a fitting symbol is found but it is not exported
            // (That diagnostic shouldn't be displayed immediately, but only when no fitting symbol is found in all imported modules)
            if (symbol->linkage_type == LinkageType::Export) {
                return std::vector<Symbol*>{symbol};
            }
        }
        return std::vector<Symbol*>{symbol};
    }

    std::vector<Symbol*> SymbolTable::ScopeEntryVisitHandler::operator()(const std::vector<SymbolId>& symbol_ids) const {
        KPL_ASSERT_NOT_NULLPTR(symbol_table);
        std::vector<Symbol*> result;
        for (const SymbolId symbol_id : symbol_ids) {
            KPL_ASSERT_THAT(symbol_id.value < symbol_table->symbols.size(),
                "Symbol count: {}, received id: {}",
                symbol_table->symbols.size(),
                symbol_id.value);
            Symbol* symbol = symbol_table->symbols[symbol_id.value];
            const SymbolKind symbol_kind = symbol->get_symbol_kind();
            KPL_ASSERT_THAT(is_overloadable_symbol(symbol_kind));
            if (is_imported_module) {
                // TODO (improvement): Maybe create a special diagnostic if a fitting symbol is found but it is not exported
                // (That diagnostic shouldn't be displayed immediately, but only when no fitting symbol is found in all imported modules)
                if (symbol->linkage_type == LinkageType::Export) {
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
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false, true);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::SymbolAlreadyExists,
                .message = std::format("Struct with name '{}' already exists in module '{}'",
                    StringPool::get().lookup(identifier_id),
                    StringPool::get().lookup(module.full_identifier_id)),
            });
        }

        Scope& scope = scopes[module.current_scope_id.value];
        const SymbolId symbol_id{.value = static_cast<uint32_t>(symbols.size())};
        scope.contained_symbols.emplace(identifier_id, symbol_id);
        Symbol* symbol = symbol_allocator.allocate<TypeSymbol>(symbol_id, module_id, scope.id, identifier_id);
        symbols.push_back(symbol);
        return symbol_id;
    }

    std::expected<SymbolId, Diagnostic> SymbolTable::create_variable(ModuleId module_id, TypeId type_id, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false, true);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            const std::string& identifier = StringPool::get().lookup(identifier_id);
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
        Symbol* symbol = symbol_allocator.allocate<VariableSymbol>(symbol_id, module_id, scope.id, type_id, identifier_id);
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
        const auto found_symbol = find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, false, true);
        KPL_ASSERT_THAT(found_symbol.has_value());
        const std::vector<Symbol*> existing_symbols = found_symbol.value();
        if (!existing_symbols.empty()) {
            const std::string& identifier = StringPool::get().lookup(identifier_id);
            const std::string& module_identifier = StringPool::get().lookup(module.full_identifier_id);
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
                        get_prototype_signature(identifier_id, parameter_type_ids, is_variadic, type_table),
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
            module_id,
            scope.id,
            type_id,
            identifier_id,
            linkage_type,
            std::move(parameter_type_ids),
            is_variadic);
        symbols.push_back(symbol);
        return symbol_id;
    }

    Module* SymbolTable::lookup(ModuleId module_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        return &modules[module_id.value];
    }

    Symbol* SymbolTable::lookup(SymbolId symbol_id) {
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), symbol_id.value);
        return symbols[symbol_id.value];
    }

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol(ModuleId module_id, StringId identifier_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return find_symbol_of_kind(module_id, identifier_id, SymbolKind::All, false, true, false);
    }

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol_of_kind(ModuleId module_id, StringId identifier_id, SymbolKind symbol_kind) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return find_symbol_of_kind(module_id, identifier_id, symbol_kind, false, true, false);
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
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, SymbolKind::All, false, false, false);
            } else {
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, SymbolKind::All, true, false, false);
            }
        } else {
            // TODO (improvement): Right now there will always be an error as soon as an ambiguous module path is found,
            // even if the wanted symbol only exists in one module.
            // It would be nice to only report a diagnostic if the symbol actually exists in multiple modules
            std::string message = std::format("Module '{}' refers to multiple modules:\n",
                get_full_identifier_from_path(module_path));
            for (size_t i = 0; i < found_modules.size(); i++) {
                message += std::format("{}  {}) {}", log::error_sub_line, i + 1, StringPool::get().lookup(found_modules[i]->full_identifier_id));
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
        bool search_imported_modules,
        bool is_recursive_search)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const Module& module = modules[module_id.value];

        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), module.current_scope_id.value);
        Scope* scope = &scopes[module.current_scope_id.value];
        std::unordered_map<ModuleId, std::vector<Symbol*>> candidate_symbol_map;
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto scope_it = scope->contained_symbols.find(identifier_id);
            if (scope_it != scope->contained_symbols.end()) {
                std::vector<Symbol*> symbols = std::visit(
                    ScopeEntryVisitHandler{
                        .symbol_table = this,
                        .is_imported_module = is_imported_module,
                    },
                    scope_it->second);
                if (!symbols.empty()) {
                    auto result_it = candidate_symbol_map.find(module_id);
                    if (result_it == candidate_symbol_map.end()) {
                        candidate_symbol_map.emplace(module_id, std::move(symbols));
                    } else {
                        result_it->second.insert(result_it->second.end(), std::make_move_iterator(symbols.begin()), std::make_move_iterator(symbols.end()));
                    }
                }
            }

            if (scope->parent_id == ScopeId::invalid()) {
                break;
            }
            KPL_ASSERT_THAT(scope->parent_id.value < scopes.size(), "Scope count: {}, received id: {}", scopes.size(), scope->parent_id.value);
            scope = &scopes[scope->parent_id.value];
        }

        if (search_imported_modules) {
            for (ModuleId imported_module_id : module.imported_module_ids) {
                KPL_ASSERT_THAT(imported_module_id.value < modules.size(),
                    "Module count: {}, received id: {}",
                    modules.size(),
                    imported_module_id.value);
                // If recursive imports are wanted, the second false here needs to be changed to true
                const auto imported_candidate_symbols = find_symbol_of_kind(imported_module_id, identifier_id, requested_symbol_kind, true, false, true);
                KPL_ASSERT_THAT(imported_candidate_symbols.has_value()); // find only returns a diagnostic if imported symbols are searched
                // Every found symbol is of the same kind, so only checking if the first symbol kind is correct is enough
                // This is the case because every imported symbol only searches itself, which means it can either find
                // - Exactly one variable with the given identifier
                // - Exactly one type with the given identifier
                // - Multiple prototypes with the given identifier
                if (!imported_candidate_symbols.value().empty()) {
                    candidate_symbol_map.emplace(imported_module_id, std::move(imported_candidate_symbols.value()));
                }
            }
        }

        if (candidate_symbol_map.empty()) {
            return {};
        }

        // Recursive searches are part of larger searches, so they should just accumulate the candidate symbols and don't report a diagnostic
        if (is_recursive_search) {
            std::vector<Symbol*> result;
            for (const auto& [module_id, candidate_symbols] : candidate_symbol_map) {
                result.insert(result.end(), std::make_move_iterator(candidate_symbols.begin()), std::make_move_iterator(candidate_symbols.end()));
            }
            return result;
        }

        // If the searched identifier is found in at least one module, it's potentially ambiguous
        // The expection to this rule are overloadable symbols, which means that the identifier is allowed to match multiple prototype symbols
        std::unordered_set<SymbolKind> found_candidate_symbol_kinds;
        int longest_candidate_identifier = std::string("Signature").size();
        int longest_candidate_symbol_kind = std::string("Kind").size();
        bool ambiguous_symbols_found = false;
        for (const auto& [candidate_module_id, candidate_symbols] : candidate_symbol_map) {
            Module& candidate_module = modules[candidate_module_id.value];
            for (const Symbol* candidate_symbol : candidate_symbols) {
                // Record module identifier for diagnostic
                const SymbolKind candidate_symbol_kind = candidate_symbol->get_symbol_kind();
                size_t candidate_symbol_size = 0;
                if (candidate_symbol_kind == SymbolKind::Prototype) {
                    const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);
                    const std::string prototype_signature = get_prototype_signature(candidate_prototype_symbol->identifier_id,
                        candidate_prototype_symbol->parameter_type_ids,
                        candidate_prototype_symbol->is_variadic,
                        type_table);
                    candidate_symbol_size = prototype_signature.size();
                } else {
                    candidate_symbol_size = StringPool::get().lookup(candidate_symbol->identifier_id).size();
                }
                // Check lengths for diagnostic formatting
                if (longest_candidate_identifier < candidate_symbol_size) {
                    longest_candidate_identifier = candidate_symbol_size;
                }
                const size_t candidate_symbol_kind_identifier_size = get_short_symbol_kind_identifier(candidate_symbol_kind).size();
                if (longest_candidate_symbol_kind < candidate_symbol_kind_identifier_size) {
                    longest_candidate_symbol_kind = candidate_symbol_kind_identifier_size;
                }

                // Check if ambiguous symbol
                if (candidate_symbol_kind != requested_symbol_kind && requested_symbol_kind != SymbolKind::All) {
                    ambiguous_symbols_found = true;
                }
                if (found_candidate_symbol_kinds.contains(candidate_symbol_kind)) {
                    if (!is_overloadable_symbol(candidate_symbol_kind)) {
                        ambiguous_symbols_found = true;
                    }
                } else {
                    found_candidate_symbol_kinds.emplace(candidate_symbol_kind);
                }
            }
        }

        // Move all found candidates into one list
        std::vector<Symbol*> result;
        for (const auto& [module_id, candidate_symbols] : candidate_symbol_map) {
            result.insert(result.end(), std::make_move_iterator(candidate_symbols.begin()), std::make_move_iterator(candidate_symbols.end()));
        }

        // No ambiguous symbols found -> return all of them
        if (!ambiguous_symbols_found) {
            return result;
        }

        // Ambiguous symbols found -> return diagnostic
        std::sort(result.begin(), result.end(), SymbolDiagnosticSorter{.symbol_table = this, .type_table = &type_table});
        const uint32_t highest_numbering_digit_count = get_digit_count(result.size());
        std::string message = std::format("Identifier '{}' refers to multiple symbols:\n{}    {}{}Kind  {:{}}  Module{}\n",
            StringPool::get().lookup(identifier_id),
            log::error_sub_line,
            std::string(highest_numbering_digit_count, ' '),
            ansi_codes::dim,
            "Signature",
            longest_candidate_identifier,
            ansi_codes::reset);
        for (size_t i = 0; i < result.size(); i++) {
            const Symbol* candidate_symbol = result[i];
            const Module& candidate_module = modules[candidate_symbol->module_id.value];
            std::string candidate_symbol_identifier;
            if (candidate_symbol->get_symbol_kind() == SymbolKind::Prototype) {
                const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);
                candidate_symbol_identifier = get_prototype_signature(candidate_prototype_symbol->identifier_id,
                    candidate_prototype_symbol->parameter_type_ids,
                    candidate_prototype_symbol->is_variadic,
                    type_table);
            } else {
                candidate_symbol_identifier = StringPool::get().lookup(candidate_symbol->identifier_id);
            }
            const uint32_t numbering = i + 1;
            const std::string candidate_symbol_kind_identifier = std::format("{}", candidate_symbol->get_symbol_kind());
            message += std::format("{}  {}) {:{}}{}  {:{}}  {}{}\n",
                log::error_sub_line,
                numbering,
                "",
                highest_numbering_digit_count - get_digit_count(numbering),
                candidate_symbol_kind_identifier.substr(0, 4),
                std::move(candidate_symbol_identifier),
                longest_candidate_identifier,
                StringPool::get().lookup(candidate_module.full_identifier_id),
                candidate_module.id == module_id ? " (self)" : "");
        }
        const Module& imported_module = modules[candidate_symbol_map.begin()->first.value];
        message += std::format("{}Use the fully qualified name of a candidate (e. g. '{}::{}...')",
            log::error_sub_line,
            StringPool::get().lookup(imported_module.full_identifier_id),
            StringPool::get().lookup(identifier_id));
        return std::unexpected(Diagnostic{.code = DiagnosticCode::AmbiguousSymbolImport, .message = std::move(message)});
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
