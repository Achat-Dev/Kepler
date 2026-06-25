// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler::semantic_analysis {

    using StringId = uint32_t;

    class StringTable {
    public:
        StringTable(const StringTable& string_table) = delete;
        void operator=(const StringTable& string_table) = delete;

        StringId store_or_lookup(const std::string& string);
        const std::string& lookup(StringId id) const;

        static StringTable& get();

    private:
        std::vector<const std::string*> strings;
        std::unordered_map<std::string, StringId> string_to_id_map;

        StringTable() = default;
    };

}