// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <format>
#include <ostream>
#include <string>
#include <string_view>

namespace kepler::type_system {

    enum class DataTypeKind {
        None,
        Void,
        TMap,
        Bool,
        Char,
        String,
        Int8,
        Int16,
        Int32,
        Int64,
        Float32,
        Float64
    };

    bool is_integer_type(DataTypeKind data_type);
    std::ostream& operator<<(std::ostream& os, DataTypeKind type);

    std::string_view to_string(DataTypeKind data_type);

}

template <>
struct std::formatter<kepler::type_system::DataTypeKind> : std::formatter<std::string> {
    auto format(const kepler::type_system::DataTypeKind& data_type_kind, std::format_context& ctx) const {
        return std::formatter<std::string>::format(std::format("{}", to_string(data_type_kind)), ctx);
    }
};
