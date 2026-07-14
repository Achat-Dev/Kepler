// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "emergency.hpp"
#include "log.hpp"
#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/data_type_kind.hpp"
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::semantic_analysis {

    std::expected<SymbolId, diagnostics::Diagnostic> SymbolTable::create_variable(const std::string& identifier, type_system::DataTypeKind data_type) {
        // Check if variable with that name already exists in the current scope
        if (does_name_exist_in_scope_stack(identifier)) {
            return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, std::format("Variable with name '{}' already exists in the current scope", identifier)));
        }

        // Create new symbol
        const SymbolId symbol_id = symbols.size();
        const Symbol symbol{
            .identifier_id = identifier,
            .data_type = data_type};
        symbols.push_back(std::move(symbol));
        const std::shared_ptr<Scope> current_scope = scopes.back();
        current_scope->contained_identifiers.emplace(identifier, symbol_id);
        return symbol_id;
    }

    std::expected<SymbolId, diagnostics::Diagnostic> SymbolTable::create_prototype(const std::string& identifier, type_system::DataTypeKind data_type, PrototypeSymbolData data) {
        // Check if prototype with that name already exists in the global scope
        // Use the global scope because functions can only be defined inside the global scope right now
        const std::shared_ptr<Scope> current_scope = scopes.back()->parent;
        if (current_scope != scopes[0]) {
            emergency_exit("Trying to create a prototype symbol in a scope other than the global scope. Current scope is '{}', expected '1' for the global scope.\n{}Catching this mistake is not my job, parser, what are you doing?", scopes.size(), log::styling::last_indented);
        }

        std::unordered_map<std::string, SymbolId>& identifiers_in_current_scope = current_scope->contained_identifiers;
        if (identifiers_in_current_scope.contains(identifier)) {
            const SymbolId symbol_id = identifiers_in_current_scope[identifier];
            const Symbol& symbol = symbols[symbol_id];

            if (std::holds_alternative<PrototypeSymbolData>(symbol.data)) {
                const std::string message = std::format("Function with name '{}' already exists", identifier);
                if (std::get<PrototypeSymbolData>(symbol.data) != data) {
                    return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, std::format("{}{}The function signature is different, but function overloading is not supported yet", message, log::styling::last_indented)));
                }
                return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, message));
            } else {
                return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::SymbolAlreadyExists, std::format("Trying to define a function with the name '{}', but a variable with that name already exists", identifier)));
            }
        }

        // Create symbol
        const SymbolId symbol_id = symbols.size();
        const Symbol symbol{
            .identifier_id = identifier,
            .data_type = data_type,
            .data = std::move(data)};
        symbols.push_back(std::move(symbol));
        identifiers_in_current_scope.emplace(identifier, symbol_id);
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

    bool SymbolTable::does_name_exist_in_scope_stack(const std::string& identifier) const {
        std::shared_ptr<Scope> scope = scopes.back();
        while (scope != nullptr) {
            if (scope->contained_identifiers.contains(identifier)) {
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
