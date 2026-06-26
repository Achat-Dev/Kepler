// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/string_table.hpp"
#include "emergency.hpp"
#include "log.hpp"
#include <string>

namespace kepler::semantic_analysis {

    StringId StringTable::store_or_lookup(const std::string& string) {
        if (string_to_id_map.contains(string)) {
            return string_to_id_map[string];
        }

        StringId id = string_to_id_map.size();
        log_verbose("[ StringTable ]: Storing string '", string, "' with id '", id, "'");
        auto [iterator, inserted] = string_to_id_map.emplace(string, id);
        strings.push_back(&iterator->first);
        return id;
    }

    const std::string& StringTable::lookup(StringId id) const {
        log_verbose("[ StringTable ]: Looking up string for id '", id, "'");
        if (id >= strings.size()) {
            log(log_type::INTERNAL_ERROR, "StringId '", id, "' doesnt exist, which is strange, because I am the only one who assigns these ids");
            emergency_exit();
        }
        return *strings[id];
    }

    StringTable& StringTable::get() {
        static StringTable instance;
        return instance;
    }

}
