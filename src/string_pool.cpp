// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "string_pool.hpp"
#include "assert.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace kepler {

    StringId StringPool::store(std::string string) {
        if (string_to_id_map.contains(string)) {
            return string_to_id_map[string];
        }

        const uint32_t interned_string_count = strings.size();
        const StringId id = StringId{interned_string_count};
        const auto [it, emplaced] = string_to_id_map.emplace(std::move(string), id);
        strings.push_back(it->first);
        return id;
    }

    std::string_view StringPool::lookup(StringId id) const {
        KPL_ASSERT(id.value < strings.size(), "String with id '{}' doesn't exist", id);
        return strings[id.value];
    }

    StringPool& StringPool::get() {
        static StringPool instance;
        return instance;
    }

}
