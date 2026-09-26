// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/struct.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/string_pool.hpp"
#include <vector>

namespace kepler {

    class TypeTable {
    public:
        inline static struct {
            TypeId unknown_type_id;
            TypeId void_type_id;
            TypeId bool_type_id;
            TypeId string_type_id;
            TypeId i8_type_id;
            TypeId i16_type_id;
            TypeId i32_type_id;
            TypeId i64_type_id;
            TypeId u8_type_id;
            TypeId u16_type_id;
            TypeId u32_type_id;
            TypeId u64_type_id;
            TypeId f32_type_id;
            TypeId f64_type_id;
        } Builtins{};

        explicit TypeTable(SymbolTable& symbol_table);
        TypeId create_struct(StringId identifier_id, std::vector<StructMember> members);
        Type* lookup(TypeId type_id);

    private:
        std::vector<Type*> types;
        ArenaAllocator allocator;

        void create_builtin_types(SymbolTable& symbol_table);
        void register_builtin_type(TypeId& type_id, TypeKind type_kind);
        void add_methods_to_builtin_number_type(TypeId type_id);
    };

}
