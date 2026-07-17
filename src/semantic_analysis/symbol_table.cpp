// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include "emergency.hpp"
#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define INVALID_SYMBOL_INDEX UINT32_MAX

namespace kepler::semantic_analysis {

    SymbolTable::SymbolTable() {
        open_scope(ScopeType::File);
    }

    std::expected<const Symbol*, diagnostics::SourceDiagnostic> SymbolTable::create_variable(type_system::DataTypeKind data_type, const std::string& identifier, const diagnostics::SourceLocation& source_location) {
        return create_symbol(data_type, identifier, std::monostate{}, "Variable", source_location);
    }

    std::expected<const Symbol*, diagnostics::SourceDiagnostic> SymbolTable::create_prototype(type_system::DataTypeKind data_type, const std::string& identifier, ast::Prototype::LinkageType linkage_type, std::vector<type_system::DataTypeKind> parameter_data_types, const diagnostics::SourceLocation& source_location) {
        return create_symbol(data_type, identifier, PrototypeSymbolData(linkage_type, std::move(parameter_data_types)), "Function", source_location);
    }

    const Symbol* SymbolTable::lookup(const std::string& identifier) const {
        const auto it = visible_symbols.find(identifier);
        if (it == visible_symbols.end()) {
            return nullptr;
        }
        return &symbols[it->second];
    }

    void SymbolTable::open_scope(ScopeType type) {
        if (scopes.empty()) {
            scopes.emplace_back(type, ScopeID{INVALID_SYMBOL_INDEX});
        } else {
            scopes.emplace_back(type, scopes.back().id);
        }
    }

    void SymbolTable::close_scope() {
        if (scopes.empty()) {
            emergency_exit("Trying to close the global scope, how did I even end up here?");
        }

        const Scope& current_scope = scopes.back();
        for (const uint32_t symbol_index : current_scope.symbol_indices) {
            const Symbol& symbol = symbols[symbol_index];
            if (symbol.shadowed_symbol_index == INVALID_SYMBOL_INDEX) {
                visible_symbols.erase(symbol.identifier);
            } else {
                visible_symbols[symbol.identifier] = symbol.shadowed_symbol_index;
            }
        }

        scopes.pop_back();
    }

    std::expected<const Symbol*, diagnostics::SourceDiagnostic> SymbolTable::create_symbol(type_system::DataTypeKind data_type, const std::string& identifier, SymbolData&& data, const std::string& error_identifier, const diagnostics::SourceLocation& source_location) {
        const Scope& current_scope = scopes.back();
        const auto it = visible_symbols.find(identifier);
        uint32_t symbol_index_to_shadow = INVALID_SYMBOL_INDEX;

        if (it != visible_symbols.end()) {
            const Symbol& symbol = symbols[it->second];
            if (symbol.scope_id.value == current_scope.id.value) {
                return std::unexpected(diagnostics::SourceDiagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, std::format("{} with name '{}' already exists in the current scope", error_identifier, symbol.identifier), source_location));
            }
            if (!symbol.can_be_shadowed) {
                return std::unexpected(diagnostics::SourceDiagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, std::format("{} with name '{}' already exists and cannot be shadowed", error_identifier, symbol.identifier), source_location));
            }
            symbol_index_to_shadow = it->second;
        }

        visible_symbols[identifier] = symbols.size();

        bool can_be_shadowed = current_scope.type != ScopeType::Function && current_scope.type != ScopeType::Block;
        symbols.emplace_back(current_scope.id, data_type, identifier, can_be_shadowed, symbol_index_to_shadow, std::move(data));
        return &symbols.back();
    }
}
