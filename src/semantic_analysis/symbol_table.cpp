// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol_table.hpp"
#include "emergency.hpp"
#include "error_code.hpp"
#include "log.hpp"
#include "semantic_analysis/prototype_symbol_data.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdlib>
#include <expected>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::semantic_analysis {

    std::expected<SymbolId, ErrorCode> SymbolTable::create_variable(StringId identifier_id, type_system::DataTypeKind data_type) {
        const std::string& identifier = StringTable::get().lookup(identifier_id);
        log_verbose("[ SymbolTable ]: Creating variable symbol of type '", data_type, "' with identifier '", identifier, "'");

        // Check if variable with that name already exists in the current scope
        if (does_name_exist_in_scope_stack(identifier_id)) {
            log(log_type::PARSING_ERROR, "Variable with name '", identifier, "' already exists in the current scope");
            return std::unexpected(ErrorCode::SymbolTableRedefineSymbol);
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

    std::expected<SymbolId, ErrorCode> SymbolTable::create_prototype(StringId identifier_id, type_system::DataTypeKind data_type, PrototypeSymbolData data) {
        const std::string& identifier = StringTable::get().lookup(identifier_id);
        log_verbose("[ SymbolTable ]: Creating prototype symbol with return type '", data_type, "' and identifier '", identifier, "'");

        // Check if prototype with that name already exists in the global scope
        // Use the global scope because functions can only be defined inside the global scope right now
        const std::shared_ptr<Scope> current_scope = scopes.back()->parent;
        if (current_scope != scopes[0]) {
            log(log_type::INTERNAL_ERROR, "Trying to create a prototype symbol in a scope other than the global scope. Current scope is '", scopes.size(), "', expected '1' for the global scope.\n",
                log_type::LAST_INDENTED, "Catching this mistake is not my job, parser, what are you doing?");
            emergency_exit();
        }

        std::unordered_map<StringId, SymbolId>& identifiers_in_current_scope = current_scope->contained_identifiers;
        if (identifiers_in_current_scope.contains(identifier_id)) {
            const SymbolId symbol_id = identifiers_in_current_scope[identifier_id];
            const Symbol& symbol = symbols[symbol_id];

            if (std::holds_alternative<PrototypeSymbolData>(symbol.data)) {
                log(log_type::PARSING_ERROR, "Function with name '", identifier, "' already exists");

                if (std::get<PrototypeSymbolData>(symbol.data) != data) {
                    log(log_type::LAST_INDENTED, "The function signature is different, but function overloading is not supported yet");
                }
            } else {
                log(log_type::PARSING_ERROR, "Trying to define a function with the name '", identifier, "', but a varibale with that name already exists");
            }

            return std::unexpected(ErrorCode::SymbolTableRedefineSymbol);
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

        log(log_type::INTERNAL_ERROR, "Symbol '", id, "' doesn't exist, which is strange, because I am the only one who assigns these ids");
        emergency_exit();
        std::abort();
    }

    void SymbolTable::open_scope() {
        log_verbose("[ SymbolTable ]: Opening new scope with id '", scopes.size(), "'");
        std::shared_ptr<Scope> scope = scopes.empty() ? std::make_shared<Scope>(nullptr) : std::make_shared<Scope>(scopes.back());
        scopes.push_back(std::move(scope));
    }

    void SymbolTable::close_scope() {
        log_verbose("[ SymbolTable ]: Closing scope with id '", scopes.size(), "'");
        if (scopes.empty()) {
            log(log_type::INTERNAL_ERROR, "Trying to close the global scope, how did I even end up here?");
            emergency_exit();
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
