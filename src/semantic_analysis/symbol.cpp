// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace kepler {

    std::string get_prototype_symbol_signature(const PrototypeSymbol* prototype_symbol, TypeTable& type_table) {
        std::string signature = std::string(StringPool::get().lookup(prototype_symbol->identifier_id)) + '(';
        for (size_t i = 0; i < prototype_symbol->parameter_type_ids.size(); i++) {
            const Type* parameter_type = type_table.lookup(prototype_symbol->parameter_type_ids[i]);
            signature += std::string(StringPool::get().lookup(parameter_type->identifier_id));
            if (i < prototype_symbol->parameter_type_ids.size() - 1) {
                signature += ',';
            }
        }
        if (prototype_symbol->is_variadic) {
            signature += ",...";
        }
        signature += ')';
        return signature;
    }

}
