// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/string_pool.hpp"

namespace kepler {

    class TypeTable {
    public:
        inline static struct {
            Type* unknown_type;
            Type* void_type;
            Type* bool_type;
            Type* string_type;
            Type* i8_type;
            Type* i16_type;
            Type* i32_type;
            Type* i64_type;
            Type* f32_type;
            Type* f64_type;
        } Builtins{};

        TypeTable();
        Type* lookup(StringId name_id) const;

    private:
        std::unordered_map<StringId, Type*> existing_types;
        ArenaAllocator allocator;

        void create_builtin_types();
        void register_builtin_type(Type** type_pointer, TypeKind type_kind);
        void add_methods_to_builtin_number_type(Type* type);
    };

}
