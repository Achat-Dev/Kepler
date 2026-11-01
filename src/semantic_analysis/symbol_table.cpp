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
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/log.hpp"
#include "utils/string_pool.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    SymbolTable::SymbolTable(TypeTable& type_table)
        : type_table(type_table), symbol_allocator(kibibyte_size * 32, "symbols") {
        // Create the root module, which acts as the container for all other modules
        // This makes it easier to search for modules
        // The identifier of the root module is empty so that it can never be extended or imported by the user
        modules.push_back({.id = {.value = 0}, .full_identifier_id = StringPool::get().store("")});

        // Create the global module, which is implicitely imported by all modules and contains all builtin types and functions
        const ModuleId global_module_id{.value = 1};
        modules.push_back({.id = global_module_id, .full_identifier_id = StringPool::get().store("__global")});
        open_scope(global_module_id, ScopeType::File);

        // Create builtin type symbols
        // Not sure if a void type symbol is needed
        /*create_builtin_type_symbol(type_table.Builtins.void_type_id);
        const SymbolId bool_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.bool_type_id);
        create_builtin_type_symbol(type_table.Builtins.string_type_id);
        const SymbolId i8_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.i8_type_id);
        const SymbolId i16_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.i16_type_id);
        const SymbolId i32_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.i32_type_id);
        const SymbolId i64_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.i64_type_id);
        const SymbolId u8_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.u8_type_id);
        const SymbolId u16_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.u16_type_id);
        const SymbolId u32_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.u32_type_id);
        const SymbolId u64_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.u64_type_id);
        const SymbolId f32_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.f32_type_id);
        const SymbolId f64_type_symbol_id = create_builtin_type_symbol(type_table.Builtins.f64_type_id);

        // Create builtin type methods
        create_builtin_bool_type_methods(bool_type_symbol_id);
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[i8_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[i16_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[i32_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[i64_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[u8_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[u16_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[u32_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[u64_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[f32_type_symbol_id.value]));
        create_builtin_number_type_methods(static_cast<TypeSymbol*>(symbols[f64_type_symbol_id.value]));*/
    }

    /*SymbolId SymbolTable::create_builtin_type_symbol(TypeId type_id) {
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        const ModuleId global_module_id = get_global_module()->id;
        const StringId type_identifier_id = type_table.lookup(type_id)->identifier_id;
        const auto type_symbol_id = create_struct(global_module_id, type_identifier_id);
        KPL_ASSERT_THAT(type_symbol_id.has_value());
        KPL_ASSERT_THAT(type_symbol_id.value().value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), type_symbol_id.value().value);
        TypeSymbol* type_symbol = static_cast<TypeSymbol*>(symbols[type_symbol_id.value().value]);
        type_symbol->type_id = type_id;
        return type_symbol->id;
    }

    void SymbolTable::create_builtin_bool_type_methods(SymbolId type_symbol_id) {
        KPL_ASSERT_THAT(type_symbol_id != SymbolId::invalid());
        const ModuleId global_module_id = get_global_module()->id;

        // Create operator methods
        const StringId a_identifier_id = StringPool::get().store("a");
        const StringId b_identifier_id = StringPool::get().store("b");
        const TypeId bool_type_id = type_table.Builtins.bool_type_id;
        const std::vector<TypeId> parameter_type_ids{bool_type_id, bool_type_id};
        const std::vector<StringId> parameter_identifier_ids{a_identifier_id, b_identifier_id};
        const auto equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::Equals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(equals_symbol_id.has_value());

        const auto not_equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::NotEquals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(not_equals_symbol_id.has_value());

        // Create cast methods
        const StringId value_identifier_id = StringPool::get().store("value");
        const StringId cast_identifier_id = StringPool::get().store("__cast");
        const std::vector<TypeId> number_type_ids{
            type_table.Builtins.i8_type_id,
            type_table.Builtins.i16_type_id,
            type_table.Builtins.i32_type_id,
            type_table.Builtins.i64_type_id,
            type_table.Builtins.u8_type_id,
            type_table.Builtins.u16_type_id,
            type_table.Builtins.u32_type_id,
            type_table.Builtins.u64_type_id,
            type_table.Builtins.f32_type_id,
            type_table.Builtins.f64_type_id,
        };
        for (TypeId number_type_id : number_type_ids) {
            const auto cast_symbol_id = create_prototype(global_module_id, bool_type_id, cast_identifier_id,
                LinkageType::Export, {number_type_id}, {value_identifier_id}, false, type_symbol_id);
        }
    }

    void SymbolTable::create_builtin_number_type_methods(TypeSymbol* type_symbol) {
        KPL_ASSERT_NOT_NULLPTR(type_symbol);
        const ModuleId global_module_id = get_global_module()->id;
        const TypeId type_id = type_symbol->type_id;
        const SymbolId type_symbol_id = type_symbol->id;

        // Create operator methods
        const StringId a_identifier_id = StringPool::get().store("a");
        const StringId b_identifier_id = StringPool::get().store("b");
        const StringId value_identifier_id = StringPool::get().store("value");
        const std::vector<TypeId> parameter_type_ids{type_id, type_id};
        const std::vector<StringId> parameter_identifier_ids{a_identifier_id, b_identifier_id};
        // Mathematical operators
        const auto add_symbol_id = create_prototype(global_module_id, type_id, get_operator_name_id(OperatorType::Plus),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(add_symbol_id.has_value());

        const auto sub_symbol_id = create_prototype(global_module_id, type_id, get_operator_name_id(OperatorType::Minus),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(sub_symbol_id.has_value());

        const auto mul_symbol_id = create_prototype(global_module_id, type_id, get_operator_name_id(OperatorType::Multiplication),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(mul_symbol_id.has_value());

        const auto div_symbol_id = create_prototype(global_module_id, type_id, get_operator_name_id(OperatorType::Division),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(div_symbol_id.has_value());

        const auto math_neg_symbol_id = create_prototype(global_module_id, type_id, StringPool::get().store("__math_ned"),
            LinkageType::Export, {type_id}, {value_identifier_id}, false, type_symbol_id);
        KPL_ASSERT_THAT(math_neg_symbol_id.has_value());

        // Comparison operators
        const TypeId bool_type_id = type_table.Builtins.bool_type_id;
        const auto less_than_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::LessThan),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(less_than_symbol_id.has_value());

        const auto greater_than_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::GreaterThan),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(greater_than_symbol_id.has_value());

        const auto equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::Equals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(equals_symbol_id.has_value());

        const auto not_equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::NotEquals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(not_equals_symbol_id.has_value());

        const auto less_equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::LessEquals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(less_equals_symbol_id.has_value());

        const auto greater_equals_symbol_id = create_prototype(global_module_id, bool_type_id, get_operator_name_id(OperatorType::GreaterEquals),
            LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id);
        KPL_ASSERT_THAT(greater_equals_symbol_id.has_value());

        // Create cast methods
        const StringId cast_identifier_id = StringPool::get().store("__cast");
        std::vector<TypeId> number_type_ids = {
            type_table.Builtins.i8_type_id,
            type_table.Builtins.i16_type_id,
            type_table.Builtins.i32_type_id,
            type_table.Builtins.i64_type_id,
            type_table.Builtins.u8_type_id,
            type_table.Builtins.u16_type_id,
            type_table.Builtins.u32_type_id,
            type_table.Builtins.u64_type_id,
            type_table.Builtins.f32_type_id,
            type_table.Builtins.f64_type_id,
        };
        for (TypeId number_type_id : number_type_ids) {
            if (number_type_id == type_id) {
                continue;
            }
            const auto cast_symbol_id = create_prototype(global_module_id, type_id, cast_identifier_id,
                LinkageType::Export, {number_type_id}, {value_identifier_id}, false, type_symbol_id);
        }
    }*/

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
        Module* module = get_root_module();
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
        KPL_ASSERT_THAT(module != get_root_module());
        module->imported_module_ids.push_back(get_global_module()->id);
        return module->id;
    }

    std::expected<ModuleId, Diagnostic> SymbolTable::register_imported_module(ModuleId module_id, const IdentifierPath& imported_module_path) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!imported_module_path.identifier_parts.empty());
        Module* imported_module = find_submodule(get_root_module(), imported_module_path);
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
        TypeId return_type_id,
        StringId identifier_id,
        LinkageType linkage_type,
        std::vector<TypeId> parameter_type_ids,
        std::vector<StringId> parameter_type_identifier_ids,
        bool is_variadic,
        SymbolId type_symbol_id)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(return_type_id != TypeId::invalid());
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
            return_type_id,
            identifier_id,
            linkage_type,
            std::move(parameter_type_ids),
            is_variadic);
        if (type_symbol_id != SymbolId::invalid()) {
            KPL_ASSERT_THAT(type_symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), type_symbol_id.value);
            TypeSymbol* type_symbol = static_cast<TypeSymbol*>(symbols[type_symbol_id.value]);
            type_symbol->method_symbol_ids.push_back(symbol_id);
        }
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

    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol_of_kind(ModuleId module_id, StringId identifier_id, SymbolKind symbol_kind) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return find_symbol_of_kind(module_id, identifier_id, symbol_kind, false, true, false);
    }

    // clang-format off
    std::expected<std::vector<Symbol*>, Diagnostic> SymbolTable::find_symbol_of_kind(ModuleId module_id,
        const IdentifierPath& module_path,
        StringId identifier_id,
        SymbolKind symbol_kind)
    {
        // clang-format on
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(!module_path.identifier_parts.empty());
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        std::vector<Module*> found_modules;
        Module* fully_qualified_module = find_submodule(get_root_module(), module_path);
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
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, symbol_kind, false, false, false);
            } else {
                return find_symbol_of_kind(found_modules[0]->id, identifier_id, symbol_kind, true, false, false);
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

        // Move all found candidates into one list for further processing
        std::vector<Symbol*> result;
        for (const auto& [module_id, candidate_symbols] : candidate_symbol_map) {
            result.insert(result.end(), std::make_move_iterator(candidate_symbols.begin()), std::make_move_iterator(candidate_symbols.end()));
        }

        // Recursive searches are part of larger searches, so they should just accumulate the candidate symbols and don't report a diagnostic
        if (is_recursive_search) {
            return result;
        }

        // If the searched identifier is found in at least one module, it's potentially ambiguous
        // The expection to this rule are overloadable symbols, which means that the identifier is allowed to match multiple prototype symbols
        std::unordered_set<SymbolKind> found_candidate_symbol_kinds;
        bool ambiguous_symbols_found = false;
        for (const Symbol* candidate_symbol : result) {
            const SymbolKind candidate_symbol_kind = candidate_symbol->get_symbol_kind();
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

        // No ambiguous symbols found -> return all of them
        if (!ambiguous_symbols_found) {
            return result;
        }

        // Ambiguous symbols found -> return diagnostic
        std::string message = std::format("Identifier '{}' refers to multiple symbols:\n", StringPool::get().lookup(identifier_id));
        message += get_candidate_symbol_diagnostic_list(result, true, true, module_id, *this, type_table);
        const Module& imported_module = modules[candidate_symbol_map.begin()->first.value];
        message += std::format("\n{}Use the fully qualified name of a candidate (e. g. '{}::{}...')",
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

    bool SymbolTable::is_symbol_visible_in_module(ModuleId module_id, SymbolId symbol_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), module_id.value);
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Symbol count: {}, received id: {}", symbols.size(), symbol_id.value);
        const Symbol* symbol = symbols[symbol_id.value];
        if (symbol->linkage_type != LinkageType::Export) {
            return false;
        }
        if (symbol->module_id == get_global_module()->id) {
            return true;
        }

        auto scope_entry_visit_handler = [symbol_id](const auto& scope_entry_value) -> bool {
            using ValueType = std::decay_t<decltype(scope_entry_value)>;
            if constexpr (std::is_same_v<ValueType, SymbolId>) {
                return scope_entry_value == symbol_id;
            } else if constexpr (std::is_same_v<ValueType, std::vector<SymbolId>>) {
                for (SymbolId scope_entry_symbol_id : scope_entry_value) {
                    if (scope_entry_symbol_id == symbol_id) {
                        return true;
                    }
                }
                return false;
            }
            KPL_ASSERT_UNREACHABLE("Missing visit implementation for ScopeEntry '{}'", typeid(scope_entry_value).name());
        };

        // Search own module
        const Module& module = modules[module_id.value];
        const Scope& module_scope = scopes[module.scope_ids.begin()->value];
        for (const auto& [identifier_id, scope_entry] : module_scope.contained_symbols) {
            const bool result = std::visit(scope_entry_visit_handler, scope_entry);
            if (result) {
                return true;
            }
        }

        // Search imported modules
        for (ModuleId imported_module_id : module.imported_module_ids) {
            KPL_ASSERT_THAT(imported_module_id.value < modules.size(), "Module count: {}, received id: {}", modules.size(), imported_module_id.value);
            const Module& imported_module = modules[imported_module_id.value];
            const Scope& imported_module_scope = scopes[imported_module.scope_ids.begin()->value];
            for (const auto& [identifier_id, scope_entry] : imported_module_scope.contained_symbols) {
                const bool result = std::visit(scope_entry_visit_handler, scope_entry);
                if (result) {
                    return true;
                }
            }
        }
        return false;
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
        KPL_ASSERT_THAT(modules.size() > 1);
        return &modules[1];
    }

    Module* SymbolTable::get_root_module() {
        KPL_ASSERT_THAT(!modules.empty());
        return &modules.front();
    }
}
