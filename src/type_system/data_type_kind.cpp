// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/data_type_kind.hpp"
#include "log.hpp"

#include <ostream>

namespace kepler::type_system {

    bool is_integer_type(DataTypeKind data_type) {
        return data_type == DataTypeKind::Int8 || data_type == DataTypeKind::Int16 || data_type == DataTypeKind::Int32 || data_type == DataTypeKind::Int64;
    }

    std::ostream& operator<<(std::ostream& os, DataTypeKind type) {
        switch (type) {
            case DataTypeKind::None: os << "none"; break;
            case DataTypeKind::Void: os << "void"; break;
            case DataTypeKind::TMap: os << "tmap"; break;
            case DataTypeKind::Bool: os << "bool"; break;
            case DataTypeKind::Char: os << "char"; break;
            case DataTypeKind::String: os << "string"; break;
            case DataTypeKind::Int8: os << "i8"; break;
            case DataTypeKind::Int16: os << "i16"; break;
            case DataTypeKind::Int32: os << "i32"; break;
            case DataTypeKind::Int64: os << "i64"; break;
            case DataTypeKind::Float32: os << "f32"; break;
            case DataTypeKind::Float64: os << "f64"; break;
            default: log(log_type::INTERNAL_LEXING_WARNING, "Missing implementation of operator '<<' for data type kind '", (int)type, '\''); break;
        }

        return os;
    }

}
