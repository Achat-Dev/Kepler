// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "diagnostics/diagnostics.hpp"
#include "diagnostics/error_code.hpp"
#include "emergency.hpp"
#include "log.hpp"
#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <expected>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::semantic_analysis {

    std::expected<SymbolId, diagnostics::ErrorCode> SymbolTable::create_variable(StringId identifier_id, type_system::DataTypeKind data_type) {
        const std::string& identifier = StringTable::get().lookup(identifier_id);

        // Check if variable with that name already exists in the current scope
        if (does_name_exist_in_scope_stack(identifier_id)) {
            diagnostics::info("{}Variable with name '{}' already exists in the current scope", diagnostics::ErrorCode::SymbolTableRedefineSymbol, identifier);
            return std::unexpected(diagnostics::ErrorCode::SymbolTableRedefineSymbol);
        }

        // Create new symbol
        const SymbolId symbol_id = symbols.size();
        const Symbol symbol{
            .identifier_id = identifier_id,
            .data_type = data_type};
        symbols.push_back(std::move(symbol));
        const std::shared_ptr<Scope> current_scope = scopes.back();
        current_scope->contained_identifiers.emplace(identifier_id, symbol_id);
        return symbol_id;
    }

    std::expected<SymbolId, diagnostics::ErrorCode> SymbolTable::create_prototype(StringId identifier_id, type_system::DataTypeKind data_type, PrototypeSymbolData data) {
        const std::string& identifier = StringTable::get().lookup(identifier_id);

        // Check if prototype with that name already exists in the global scope
        // Use the global scope because functions can only be defined inside the global scope right now
        const std::shared_ptr<Scope> current_scope = scopes.back()->parent;
        if (current_scope != scopes[0]) {
            emergency_exit("Trying to create a prototype symbol in a scope other than the global scope. Current scope is '{}', expected '1' for the global scope.\n{}Catching this mistake is not my job, parser, what are you doing?", scopes.size(), log::styling::last_indented);
        }

        std::unordered_map<StringId, SymbolId>& identifiers_in_current_scope = current_scope->contained_identifiers;
        if (identifiers_in_current_scope.contains(identifier_id)) {
            const SymbolId symbol_id = identifiers_in_current_scope[identifier_id];
            const Symbol& symbol = symbols[symbol_id];

            if (std::holds_alternative<PrototypeSymbolData>(symbol.data)) {
                diagnostics::info("{}Function with name '{}' already exists", diagnostics::ErrorCode::SymbolTableRedefineSymbol, identifier);

                if (std::get<PrototypeSymbolData>(symbol.data) != data) {
                    diagnostics::info("{}The function signature is different, but function overloading is not supported yet", log::styling::last_indented);
                }
            } else {
                diagnostics::info("{}Trying to define a function with the name '{}', but a variable with that name already exists", log::styling::last_indented, identifier);
            }

            return std::unexpected(diagnostics::ErrorCode::SymbolTableRedefineSymbol);
        }

        // Create symbol
        const SymbolId symbol_id = symbols.size();
        const Symbol symbol{
            .identifier_id = identifier_id,
            .data_type = data_type,
            .data = std::move(data)};
        symbols.push_back(std::move(symbol));
        identifiers_in_current_scope.emplace(identifier_id, symbol_id);
        return symbol_id;
    }

    const Symbol& SymbolTable::lookup(SymbolId id) const {
        std::shared_ptr<Scope> scope = scopes.back();
        const Symbol& symbol = symbols[id];
        while (scope != nullptr) {
            if (scope->contained_identifiers.contains(symbol.identifier_id)) {
                return symbol;
            }
            scope = scope->parent;
        }

        emergency_exit("Symbol '{}' doesn't exist, which is strange, because I am the only one who assigns these ids", id);
    }

    void SymbolTable::open_scope() {
        std::shared_ptr<Scope> scope = scopes.empty() ? std::make_shared<Scope>(nullptr) : std::make_shared<Scope>(scopes.back());
        scopes.push_back(std::move(scope));
    }

    void SymbolTable::close_scope() {
        if (scopes.empty()) {
            emergency_exit("Trying to close the global scope, how did I even end up here?");
        }
        scopes.pop_back();
    }

    bool SymbolTable::does_name_exist_in_scope_stack(StringId identifier_id) const {
        std::shared_ptr<Scope> scope = scopes.back();
        while (scope != nullptr) {
            if (scope->contained_identifiers.contains(identifier_id)) {
                return true;
            }
            scope = scope->parent;
        }

        return false;
    }

    SymbolTable& SymbolTable::get() {
        static SymbolTable instance;
        return instance;
    }

    SymbolTable::SymbolTable() {
        open_scope(); // Create the global scope
    }

}
