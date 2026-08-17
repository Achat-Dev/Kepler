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
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
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

    SymbolTable::SymbolTable() {
        open_scope(ScopeType::File);
    }

    std::expected<Symbol*, SourceDiagnostic> SymbolTable::create_variable(Type* type, StringId identifier_id, SourceLocation source_location) {
        return create_symbol(type, identifier_id, std::monostate{}, "Variable", source_location);
    }

    // clang-format off
    std::expected<Symbol*, SourceDiagnostic> SymbolTable::create_prototype(Type* type,
        StringId identifier_id,
        Prototype::LinkageType linkage_type,
        std::vector<Type*> parameter_types,
        SourceLocation identifier_source_location
    ) {
        // clang-format on
        return create_symbol(type,
            identifier_id,
            PrototypeSymbolData{.linkage_type = linkage_type, .parameter_types = std::move(parameter_types)},
            "Prototype",
            identifier_source_location);
    }

    Symbol* SymbolTable::lookup(StringId identifier_id) {
        KPL_ASSERT_THAT(!scopes.empty(), "Can't lookup a symbol when no scopes exist");
        Scope* scope = &scopes.back();
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto it = scope->contained_symbols.find(identifier_id);
            if (it != scope->contained_symbols.end()) {
                KPL_ASSERT_THAT(it->second < symbols.size(),
                    "Can't lookup symbol at out of bounds index; symbol count is {}, received index {}",
                    symbols.size(),
                    it->second);
                return &symbols[it->second];
            }

            if (scope->parent_id == ScopeId::invalid()) {
                return nullptr;
            }
            KPL_ASSERT_THAT(scope->parent_id.value < scopes.size(),
                "Can't access out of bounds parent scope; scope count is {}, received index {}",
                scopes.size(),
                scope->parent_id.value);
            scope = &scopes[scope->parent_id.value];
        }
    }

    void SymbolTable::open_scope(ScopeType type) {
        const ScopeId scope_id = {.value = static_cast<uint32_t>(scopes.size())};
        if (scopes.empty()) {
            scopes.emplace_back(type, scope_id, ScopeId::invalid(), std::unordered_map<StringId, uint32_t>{});
        } else {
            scopes.emplace_back(type, scope_id, current_scope->id, std::unordered_map<StringId, uint32_t>{});
        }
        current_scope = &scopes.back();
    }

    void SymbolTable::close_scope() {
        KPL_ASSERT_THAT(!scopes.empty(), "No scope to close exists");
        KPL_ASSERT_THAT(scopes.back().parent_id != ScopeId::invalid(), "Can't close the global scope");
        KPL_ASSERT_THAT(current_scope->parent_id.value < scopes.size(),
            "Can't close scope with out of bounds parent id; scope count is, received index {}",
            scopes.size(),
            current_scope->parent_id.value);
        current_scope = &scopes[current_scope->parent_id.value];
    }

    // clang-format off
    std::expected<Symbol*, SourceDiagnostic> SymbolTable::create_symbol(Type* type,
        StringId identifier_id,
        SymbolData&& data,
        const std::string& error_identifier,
        SourceLocation source_location
    ) {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(!error_identifier.empty(), "Can't create symbol when error identifier is empty");
        KPL_ASSERT_THAT(!scopes.empty(), "Can't create symbol when no scopes exist");

        Scope& current_scope = scopes.back();
        KPL_ASSERT_THAT(current_scope.id != ScopeId::invalid(), "Can't create symbol when current scope has invalid id");

        const auto existing_symbol = lookup_with_index(identifier_id, current_scope.id);
        uint32_t symbol_index_to_shadow = INVALID_SYMBOL_INDEX;
        if (existing_symbol.first != nullptr) {
            if (existing_symbol.first->scope_id == current_scope.id) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol.first->identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists in the current scope", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            if (!existing_symbol.first->can_be_shadowed) {
                const std::string_view identifier = StringPool::get().lookup(existing_symbol.first->identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists and cannot be shadowed", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            symbol_index_to_shadow = existing_symbol.second;
        }

        bool can_be_shadowed = current_scope.type != ScopeType::Function && current_scope.type != ScopeType::Block;
        current_scope.contained_symbols.emplace(identifier_id, symbols.size());
        symbols.emplace_back(current_scope.id, type, identifier_id, can_be_shadowed, symbol_index_to_shadow, std::move(data));
        return &symbols.back();
    }

    std::pair<Symbol*, uint32_t> SymbolTable::lookup_with_index(StringId identifier_id, ScopeId scope_id) {
        KPL_ASSERT_THAT(!scopes.empty(), "Can't lookup symbol with index when no scopes exist");
        KPL_ASSERT_THAT(scope_id != ScopeId::invalid(), "Can't lookup symbol with index in invalid scope");
        KPL_ASSERT_THAT(scope_id.value < scopes.size(),
            "Can't lookup symbol with index in out of bounds scope; scope count is {}, received index {}",
            scopes.size(),
            scope_id.value);
        Scope* scope = &scopes[scope_id.value];
        while (true) {
            KPL_ASSERT_NOT_NULLPTR(scope);
            const auto it = scope->contained_symbols.find(identifier_id);
            if (it != scope->contained_symbols.end()) {
                KPL_ASSERT_THAT(it->second < symbols.size(),
                    "Can't lookup symbol at out of bounds index; symbol count is {}, received index {}",
                    symbols.size(),
                    it->second);
                return {&symbols[it->second], it->second};
            }

            if (scope->parent_id != ScopeId::invalid()) {
                KPL_ASSERT_THAT(scope->parent_id.value < scopes.size(), "Can't access out of bounds parent scope");
                scope = &scopes[scope->parent_id.value];
            } else {
                return {nullptr, INVALID_SYMBOL_INDEX};
            }
        }
    }
}
