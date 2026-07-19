// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct StringId {
        uint32_t value;

        bool operator==(const StringId& other) const = default;
        bool operator!=(const StringId& other) const = default;
    };

    class StringPool {
    public:
        StringId store(std::string string);
        std::string_view lookup(StringId id) const;

        static StringPool& get();

    private:
        std::vector<std::string_view> strings;
        std::unordered_map<std::string, StringId> string_to_id_map;

        StringPool() = default;
        ~StringPool() = default;
        StringPool(const StringPool& other) = delete;
        StringPool& operator=(const StringPool&) = delete;
        StringPool(StringPool&&) = delete;
        StringPool& operator=(StringPool&&) = delete;
    };

}

template <>
struct std::hash<kepler::StringId> {
    size_t operator()(const kepler::StringId& id) const noexcept {
        return hash<uint32_t>{}(id.value);
    }
};

template <>
struct std::formatter<kepler::StringId> : std::formatter<std::string> {
    auto format(const kepler::StringId& id, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("{}", id.value), ctx);
    }
};
