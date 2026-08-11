// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <vector>

namespace kepler {

    const Method* Type::find_method(StringId identifier_id, std::vector<Type*> parameter_types) const {
        for (const Method& method : methods) {
            if (method.identifier_id == identifier_id && method.parameter_types == parameter_types) {
                return &method;
            }
        }
        return nullptr;
    }

    bool is_integer_type(Type& type) {
        switch (type.type_kind) {
            case TypeKind::I8:
            case TypeKind::I16:
            case TypeKind::I32:
            case TypeKind::I64:
                return true;
            default:
                return false;
        }
    }

    bool is_floating_point_type(Type& type) {
        switch (type.type_kind) {
            case TypeKind::F32:
            case TypeKind::F64:
                return true;
            default:
                return false;
        }
    }

    StringId get_type_kind_name_id(TypeKind type_kind) {
        switch (type_kind) {
            case kepler::TypeKind::Unknown:
                return StringPool::get().store("__unknown");
            case kepler::TypeKind::Void:
                return StringPool::get().store("void");
            case kepler::TypeKind::Bool:
                return StringPool::get().store("bool");
            case kepler::TypeKind::String:
                return StringPool::get().store("string");
            case kepler::TypeKind::I8:
                return StringPool::get().store("i8");
            case kepler::TypeKind::I16:
                return StringPool::get().store("i16");
            case kepler::TypeKind::I32:
                return StringPool::get().store("i32");
            case kepler::TypeKind::I64:
                return StringPool::get().store("i64");
            case kepler::TypeKind::F32:
                return StringPool::get().store("f32");
            case kepler::TypeKind::F64:
                return StringPool::get().store("f64");
        }

        assert::unreachable(std::format("Missing type name id implementation for type kind '{}'", static_cast<int>(type_kind)));
    }

}
