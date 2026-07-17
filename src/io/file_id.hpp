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
#include <format>
#include <string>

#define INVALID_FILE_ID UINT32_MAX

namespace kepler::io {

    struct FileId {
        uint32_t value;
    };

}

template <>
struct std::formatter<kepler::io::FileId> : std::formatter<std::string> {
    auto format(const kepler::io::FileId& id, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("{}", id.value), ctx);
    }
};
