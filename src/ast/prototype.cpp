// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/prototype.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
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
                } else if constexpr (std::is_same_v<ValueType, IdentifierPath>) {
                    KPL_ASSERT_THAT(prototype_identifier.identifier_parts.size() == 2);
                    return StringPool::get().store(get_full_identifier_from_path(prototype_identifier));
                }
                KPL_ASSERT_UNREACHABLE("Missing visit implementation for PrototypeIdentifier '{}'",
                    typeid(prototype_identifier).name());
            },
            prototype->identifier);
    }

}
