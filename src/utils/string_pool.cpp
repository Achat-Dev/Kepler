// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/string_pool.hpp"
#include "utils/assert.h"
#include <cstdint>
#include <string>
#include <utility>

namespace kepler {

    StringId StringPool::store(std::string string) {
        const auto it = string_to_id_map.find(string);
        if (it != string_to_id_map.end()) {
            return it->second;
        }

        const uint32_t interned_string_count = strings.size();
        const StringId id{.value = interned_string_count};
        string_to_id_map.emplace(string, id);
        strings.push_back(std::move(string));
        return id;
    }

    const std::string& StringPool::lookup(StringId id) const {
        KPL_ASSERT_THAT(id.value < strings.size(), "String count: {}, received id: {}", strings.size(), id.value);
        return strings[id.value];
    }

    StringPool& StringPool::get() {
        static StringPool instance;
        return instance;
    }

}
