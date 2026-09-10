// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "ast/prototype.hpp"
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

    ModuleId SymbolTable::create_module(std::vector<StringId> identifier_ids) {
        const uint32_t module_count = modules.size();
        for (size_t i = 0; i < module_count; i++) {
            if (modules[i].identifier_ids == identifier_ids) {
                return ModuleId{.value = static_cast<uint32_t>(i)};
            }
        }
        modules.push_back({.identifier_ids = std::move(identifier_ids)});
        const ModuleId module_id = ModuleId{.value = module_count};
        open_scope(module_id, ScopeType::File);
        return module_id;
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
        PrototypeLinkageType linkage_type,
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
        KPL_ASSERT_THAT(symbol_id.value < symbols.size(), "Can't lookup symbol with out of bounds symbol id");
        return &symbols[symbol_id.value];
    }

    Symbol* SymbolTable::find(ModuleId module_id, StringId identifier_id) {
        KPL_ASSERT_THAT(!scopes.empty(), "Can't find symbol when no scopes exist");
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Can't find symbol with out of bounds module id");
        const Module& module = modules[module_id.value];

        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Can't find symbol with out of bounds scope id");
        Scope* scope = &scopes[module.current_scope_id.value];
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto it = scope->contained_symbols.find(identifier_id);
            if (it != scope->contained_symbols.end()) {
                KPL_ASSERT_THAT(it->second < symbols.size(), "Can't find symbol with out of bounds index");
                return &symbols[it->second];
            }

            if (scope->parent_id == ScopeId::invalid()) {
                return nullptr;
            }
            KPL_ASSERT_THAT(scope->parent_id.value < scopes.size(), "Can't access out of bounds parent scope");
            scope = &scopes[scope->parent_id.value];
        }
    }

    void SymbolTable::open_scope(ModuleId module_id, ScopeType type) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Can't open scope with out of bounds module id");
        const ScopeId scope_id = {.value = static_cast<uint32_t>(scopes.size())};
        Module& module = modules[module_id.value];
        if (scopes.empty()) {
            scopes.emplace_back(type, scope_id, ScopeId::invalid(), std::unordered_map<StringId, uint32_t>{});
        } else {
            scopes.emplace_back(type, scope_id, module.current_scope_id, std::unordered_map<StringId, uint32_t>{});
        }
        module.scope_ids.push_back(scope_id);
        module.current_scope_id = scope_id;
    }

    void SymbolTable::close_scope(ModuleId module_id) {
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Can't close scope with out of bounds module id");
        KPL_ASSERT_THAT(!scopes.empty(), "No scope to close exists");
        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Can't close scope with out of bounds scope id");
        const Scope& scope = scopes[module.current_scope_id.value];
        KPL_ASSERT_THAT(scope.parent_id.value < scopes.size(), "Can't close scope with out of bounds parent id");
        module.current_scope_id = scope.parent_id;
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
        KPL_ASSERT_THAT(!scopes.empty(), "Can't create symbol when no scopes exist");
        KPL_ASSERT_THAT(module_id.value < modules.size(), "Can't create symbol with out of bounds module id");
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(!error_identifier.empty(), "Can't create symbol when error identifier is empty");

        Module& module = modules[module_id.value];
        KPL_ASSERT_THAT(module.current_scope_id.value < scopes.size(), "Can't create symbol with out of bounds scope id");

        const Symbol* existing_symbol = find(module_id, identifier_id);
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
