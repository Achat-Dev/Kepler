// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol.hpp"
#include "ast/prototype.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include "utils/log.hpp"
#include "utils/number_utils.hpp"
#include "utils/string_pool.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    bool SymbolDiagnosticSorter::operator()(const Symbol* a, const Symbol* b) const {
        KPL_ASSERT_NOT_NULLPTR(symbol_table);
        KPL_ASSERT_NOT_NULLPTR(type_table);
        KPL_ASSERT_NOT_NULLPTR(a);
        KPL_ASSERT_NOT_NULLPTR(b);
        const SymbolKind a_symbol_kind = a->get_symbol_kind();
        const SymbolKind b_symbol_kind = b->get_symbol_kind();
        // 1) Sort by symbol kind
        if (a_symbol_kind != b_symbol_kind) {
            return a_symbol_kind < b_symbol_kind;
        }

        // 2) If it's a function, sort by:
        if (a_symbol_kind == SymbolKind::Prototype && b_symbol_kind == SymbolKind::Prototype) {
            const PrototypeSymbol* a_prototype_symbol = static_cast<const PrototypeSymbol*>(a);
            const PrototypeSymbol* b_prototype_symbol = static_cast<const PrototypeSymbol*>(b);
            const size_t a_parameter_count = a_prototype_symbol->parameter_type_ids.size();
            const size_t b_parameter_count = b_prototype_symbol->parameter_type_ids.size();
            // 2.1) Sort functions by parameter count
            if (a_parameter_count != b_parameter_count) {
                return a_parameter_count < b_parameter_count;
            }
            // 2.2) Sort functions with the same amount of parameters by the parameter identifiers
            for (size_t i = 0; i < a_parameter_count; i++) {
                KPL_ASSERT_THAT(a_prototype_symbol->parameter_type_ids[i] != TypeId::invalid());
                KPL_ASSERT_THAT(b_prototype_symbol->parameter_type_ids[i] != TypeId::invalid());
                const Type* a_type = type_table->lookup(a_prototype_symbol->parameter_type_ids[i]);
                const Type* b_type = type_table->lookup(b_prototype_symbol->parameter_type_ids[i]);
                KPL_ASSERT_THAT(a_type->identifier_id != StringId::invalid());
                KPL_ASSERT_THAT(b_type->identifier_id != StringId::invalid());
                const std::string& a_parameter_identifier = StringPool::get().lookup(a_type->identifier_id);
                const std::string& b_parameter_identifier = StringPool::get().lookup(b_type->identifier_id);
                if (a_parameter_identifier != b_parameter_identifier) {
                    return a_parameter_identifier < b_parameter_identifier;
                }
            }
            // 2.3) Sort variadic functions after functions with the same parameters
            if (a_prototype_symbol->is_variadic != b_prototype_symbol->is_variadic) {
                return a_prototype_symbol->is_variadic < b_prototype_symbol->is_variadic;
            }
        }

        KPL_ASSERT_THAT(a->module_id != ModuleId::invalid());
        const Module* a_module = symbol_table->lookup(a->module_id);
        KPL_ASSERT_THAT(a_module->full_identifier_id != StringId::invalid());
        const std::string& a_module_identifier = StringPool::get().lookup(a_module->full_identifier_id);
        KPL_ASSERT_THAT(b->module_id != ModuleId::invalid());
        const Module* b_module = symbol_table->lookup(b->module_id);
        KPL_ASSERT_THAT(b_module->full_identifier_id != StringId::invalid());
        const std::string& b_module_identifier = StringPool::get().lookup(b_module->full_identifier_id);
        // 3) Sort by module identifier
        return a_module_identifier < b_module_identifier;
    }

    bool is_overloadable_symbol(SymbolKind symbol_kind) {
        switch (symbol_kind) {
            case SymbolKind::All:
                KPL_ASSERT_UNREACHABLE("Cannot determine overloadable for symbol kind '{}'", symbol_kind);
            case SymbolKind::Variable:
                return false;
            case SymbolKind::Prototype:
                return true;
            case SymbolKind::Type:
                return false;
        }
        KPL_ASSERT_UNREACHABLE("Missing is overloadable symbol implementation for symbol kind '{}'", static_cast<int>(symbol_kind));
    }

    std::string get_short_symbol_kind_identifier(SymbolKind symbol_kind) {
        switch (symbol_kind) {
            case SymbolKind::All:
                KPL_ASSERT_UNREACHABLE("Cannot determine short symbol identifier for symbol kind '{}'", symbol_kind);
            case SymbolKind::Variable:
                return "Var";
            case SymbolKind::Prototype:
                return "Func";
            case SymbolKind::Type:
                return "Type";
        }
        KPL_ASSERT_UNREACHABLE("Missing short symbol identifier implementation for symbol kind '{}'", static_cast<int>(symbol_kind));
    }

    // clang-format off
    std::string get_candidate_symbol_diagnostic_list(std::vector<Symbol*>& candidate_symbols,
        bool include_module,
        bool include_sybmol_kind,
        ModuleId self_module_id,
        SymbolTable& symbol_table,
        TypeTable& type_table)
    {
        // clang-format on
        std::vector<std::string> candidate_identifiers;
        candidate_identifiers.reserve(candidate_symbols.size());
        int longest_candidate_identifier = std::string("Signature").size();
        int longest_candidate_symbol_kind = std::string("Kind").size();
        for (const Symbol* candidate_symbol : candidate_symbols) {
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
        }

        // Ambiguous symbols found -> return diagnostic
        std::sort(candidate_symbols.begin(), candidate_symbols.end(), SymbolDiagnosticSorter{.symbol_table = &symbol_table, .type_table = &type_table});
        const uint32_t highest_numbering_digit_count = get_digit_count(candidate_symbols.size());
        std::string message;
        // Check if header is needed
        if (include_module || include_sybmol_kind) {
            message = std::format("{}    {}{}{}{:{}}{}{}\n",
                log::error_sub_line,
                std::string(highest_numbering_digit_count, ' '),
                ansi_codes::dim,
                include_sybmol_kind ? "Kind  " : "",
                "Signature",
                longest_candidate_identifier,
                include_module ? "  Module" : "",
                ansi_codes::reset);
        }

        // Append candidate identifiers
        for (size_t i = 0; i < candidate_symbols.size(); i++) {
            const Symbol* candidate_symbol = candidate_symbols[i];
            KPL_ASSERT_THAT(candidate_symbol->module_id != ModuleId::invalid());
            const Module* candidate_module = symbol_table.lookup(candidate_symbol->module_id);
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
            message += std::format("{}  {}) {:{}}", log::error_sub_line, numbering, "", highest_numbering_digit_count - get_digit_count(numbering));
            if (include_sybmol_kind) {
                message += std::format("{}  ", get_short_symbol_kind_identifier(candidate_symbol->get_symbol_kind()));
            }
            message += std::format("{:{}}", std::move(candidate_symbol_identifier), longest_candidate_identifier);
            if (include_module) {
                message += std::format("  {}{}",
                    StringPool::get().lookup(candidate_module->full_identifier_id),
                    candidate_module->id == self_module_id ? " (self)" : "");
            }
            if (i < candidate_symbols.size() - 1) {
                message += '\n';
            }
        }
        return message;
    }

}
