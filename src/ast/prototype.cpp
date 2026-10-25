// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/prototype.hpp"
#include "ast/expressions/member_expression.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <string>
#include <type_traits>
#include <variant>

namespace kepler {

    StringId get_prototype_identifier_id(const Prototype* prototype) {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        return std::visit(
            [](const auto& prototype_identifier) -> StringId {
                using ValueType = std::decay_t<decltype(prototype_identifier)>;
                if constexpr (std::is_same_v<ValueType, StringId>) {
                    KPL_ASSERT_THAT(prototype_identifier != StringId::invalid());
                    return prototype_identifier;
                } else if constexpr (std::is_same_v<ValueType, MemberFunctionIdentifier>) {
                    KPL_ASSERT_THAT(prototype_identifier.type_identifier_id != StringId::invalid());
                    KPL_ASSERT_THAT(prototype_identifier.identifier_id != StringId::invalid());
                    const std::string& type_identifier = StringPool::get().lookup(prototype_identifier.type_identifier_id);
                    const std::string& identifier = StringPool::get().lookup(prototype_identifier.identifier_id);
                    return StringPool::get().store(std::format("{}.{}", type_identifier, identifier));
                }
                KPL_ASSERT_UNREACHABLE("Missing visit implementation for PrototypeIdentifier '{}'",
                    typeid(prototype_identifier).name());
            },
            prototype->identifier);
    }

    // clang-format off
    std::string get_prototype_signature(StringId identifier_id,
        const std::vector<TypeId>& parameter_type_ids,
        bool is_variadic,
        TypeTable& type_table)
    {
        // clang-format on
        std::string signature = StringPool::get().lookup(identifier_id) + '(';
        for (size_t i = 0; i < parameter_type_ids.size(); i++) {
            const Type* parameter_type = type_table.lookup(parameter_type_ids[i]);
            signature += StringPool::get().lookup(parameter_type->identifier_id);
            if (i < parameter_type_ids.size() - 1) {
                signature += ',';
            }
        }
        if (is_variadic) {
            if (parameter_type_ids.empty()) {
                signature += "...";
            } else {
                signature += ",...";
            }
        }
        signature += ')';
        return signature;
    }

}
