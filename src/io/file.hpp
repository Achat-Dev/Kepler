// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <string>
#include <vector>

namespace kepler {

    struct FileId {
        uint32_t value = 0;

        bool operator==(const FileId& other) const = default;
        bool operator!=(const FileId& other) const = default;
    };

    struct File {
        FileId id;
        std::filesystem::path path;
        std::string content;
    };

    struct LineInfo {
        uint32_t line_number = 0;
        uint32_t start_position = 0;
        uint32_t size = 0;
    };

    std::vector<LineInfo> get_line_infos(const File* file);

}

template <>
struct std::hash<kepler::FileId> {
    size_t operator()(const kepler::FileId& id) const noexcept {
        return hash<uint32_t>{}(id.value);
    }
};

template <>
struct std::formatter<kepler::FileId> : std::formatter<std::string> {
    auto format(const kepler::FileId& id, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("{}", id.value), ctx);
    }
};
