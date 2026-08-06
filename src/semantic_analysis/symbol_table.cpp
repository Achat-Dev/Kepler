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
#include "utils/string_pool.hpp"
#include <cassert>
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

    std::expected<const Symbol*, SourceDiagnostic> SymbolTable::create_variable(Type* type, StringId identifier_id, SourceLocation source_location) {
        return create_symbol(type, identifier_id, std::monostate{}, "Variable", source_location);
    }

    // clang-format off
    std::expected<const Symbol*, SourceDiagnostic> SymbolTable::create_prototype(Type* type,
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

    // TODO: Not the best implementation
    const Symbol* SymbolTable::lookup(StringId identifier_id) const {
        for (const Symbol& symbol : symbols) {
            if (symbol.identifier_id == identifier_id) {
                return &symbol;
            }
        }
        return nullptr;
    }

    const Symbol* SymbolTable::lookup_visible(StringId identifier_id) const {
        const auto it = visible_symbols.find(identifier_id);
        if (it == visible_symbols.end()) {
            return nullptr;
        }
        return &symbols[it->second];
    }

    void SymbolTable::open_scope(ScopeType type) {
        if (scopes.empty()) {
            scopes.emplace_back(type, ScopeId::invalid());
        } else {
            scopes.emplace_back(type, scopes.back().id);
        }
    }

    void SymbolTable::close_scope() {
        assert(scopes.size() > 0 && "Can't close the global scope");
        const Scope& current_scope = scopes.back();
        for (const uint32_t symbol_index : current_scope.symbol_indices) {
            const Symbol& symbol = symbols[symbol_index];
            if (symbol.shadowed_symbol_index == INVALID_SYMBOL_INDEX) {
                visible_symbols.erase(symbol.identifier_id);
            } else {
                visible_symbols[symbol.identifier_id] = symbol.shadowed_symbol_index;
            }
        }

        scopes.pop_back();
    }

    // clang-format off
    std::expected<const Symbol*, SourceDiagnostic> SymbolTable::create_symbol(Type* type,
        StringId identifier_id,
        SymbolData&& data,
        const std::string& error_identifier,
        SourceLocation source_location
    ) {
        // clang-format on
        Scope& current_scope = scopes.back();
        const auto it = visible_symbols.find(identifier_id);
        uint32_t symbol_index_to_shadow = INVALID_SYMBOL_INDEX;

        if (it != visible_symbols.end()) {
            const Symbol& symbol = symbols[it->second];
            if (symbol.scope_id.value == current_scope.id.value) {
                const std::string_view identifier = StringPool::get().lookup(symbol.identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists in the current scope", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            if (!symbol.can_be_shadowed) {
                const std::string_view identifier = StringPool::get().lookup(symbol.identifier_id);
                return std::unexpected(SourceDiagnostic{
                    .code = DiagnosticCode::SymbolAlreadyExists,
                    .message = std::format("{} with name '{}' already exists and cannot be shadowed", error_identifier, identifier),
                    .source_location = source_location,
                });
            }
            symbol_index_to_shadow = it->second;
        }

        visible_symbols[identifier_id] = symbols.size();

        bool can_be_shadowed = current_scope.type != ScopeType::Function && current_scope.type != ScopeType::Block;
        const uint32_t symbol_index = symbols.size();
        current_scope.symbol_indices.push_back(symbol_index);
        symbols.emplace_back(current_scope.id, type, identifier_id, can_be_shadowed, symbol_index_to_shadow, std::move(data));
        return &symbols.back();
    }
}
