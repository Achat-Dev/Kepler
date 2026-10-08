// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/identifier_path.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <string>

namespace kepler {

    bool IdentifierPathPart::operator==(const IdentifierPathPart& other) const {
        return identifier_id == other.identifier_id;
    }

    bool IdentifierPathPart::operator!=(const IdentifierPathPart& other) const {
        return identifier_id != other.identifier_id;
    }

    bool IdentifierPath::operator==(const IdentifierPath& other) const {
        return identifier_parts == other.identifier_parts;
    }

    bool IdentifierPath::operator!=(const IdentifierPath& other) const {
        return identifier_parts != other.identifier_parts;
    }

    std::string get_full_identifier_from_path(const IdentifierPath& identifier_path) {
        KPL_ASSERT_THAT(!identifier_path.identifier_parts.empty());
        if (identifier_path.separator_id == StringId::invalid()) {
            KPL_ASSERT_THAT(identifier_path.identifier_parts.size() == 1);
            return std::string(StringPool::get().lookup(identifier_path.identifier_parts[0].identifier_id));
        }
        std::string result;
        const std::string separator(StringPool::get().lookup(identifier_path.separator_id));
        for (size_t i = 0; i < identifier_path.identifier_parts.size(); i++) {
            const StringId part_identifier_id = identifier_path.identifier_parts[i].identifier_id;
            KPL_ASSERT_THAT(part_identifier_id != StringId::invalid());
            result += StringPool::get().lookup(part_identifier_id);
            if (i < identifier_path.identifier_parts.size() - 1) {
                result += separator;
            }
        }
        return result;
    }

}
