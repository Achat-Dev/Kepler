// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "log.hpp"
#include <format>
#include <string>

namespace kepler {

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

}

template <>
struct std::formatter<kepler::DataTypeKind> : std::formatter<std::string> {
    auto format(const kepler::DataTypeKind& data_type_kind, std::format_context& ctx) const {
        switch (data_type_kind) {
            case kepler::DataTypeKind::None:
                return std::formatter<std::string>::format("none", ctx);
            case kepler::DataTypeKind::Void:
                return std::formatter<std::string>::format("void", ctx);
            case kepler::DataTypeKind::Bool:
                return std::formatter<std::string>::format("bool", ctx);
            case kepler::DataTypeKind::Char:
                return std::formatter<std::string>::format("char", ctx);
            case kepler::DataTypeKind::String:
                return std::formatter<std::string>::format("string", ctx);
            case kepler::DataTypeKind::Int8:
                return std::formatter<std::string>::format("i8", ctx);
            case kepler::DataTypeKind::Int16:
                return std::formatter<std::string>::format("i16", ctx);
            case kepler::DataTypeKind::Int32:
                return std::formatter<std::string>::format("i32", ctx);
            case kepler::DataTypeKind::Int64:
                return std::formatter<std::string>::format("i64", ctx);
            case kepler::DataTypeKind::Float32:
                return std::formatter<std::string>::format("f32", ctx);
            case kepler::DataTypeKind::Float64:
                return std::formatter<std::string>::format("f64", ctx);
            default:
                kepler::log::warning("Missing implementation of operator '<<' for data type kind '{}'", static_cast<int>(data_type_kind));
                return std::formatter<std::string>::format(std::format("{}", static_cast<int>(data_type_kind)), ctx);
        }
    }
};
