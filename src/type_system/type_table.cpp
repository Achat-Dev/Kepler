// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_table.hpp"
#include "ast/struct.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace kepler {

    TypeId TypeTable::create_type(StringId identifier_id, TypeKind type_kind, SymbolId symbol_id) {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const TypeId type_id{.value = static_cast<uint32_t>(types.size())};
        types.push_back(allocator.allocate<Type>(type_id, type_kind, identifier_id, symbol_id));
        return type_id;
    }

    TypeId TypeTable::create_struct(StringId identifier_id, SymbolId symbol_id) {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const TypeId type_id{.value = static_cast<uint32_t>(types.size())};
        types.push_back(allocator.allocate<StructType>(type_id, TypeKind::Struct, identifier_id, symbol_id, std::vector<StructTypeMember>{}));
        return type_id;
    }

    void TypeTable::create_struct_members(TypeId struct_type_id, const std::vector<StructNodeMember>& members) {
        KPL_ASSERT_THAT(struct_type_id.value < types.size(), "Type count: {}, received id: {}", types.size(), struct_type_id.value);
        KPL_ASSERT_THAT(struct_type_id != TypeId::invalid());
        std::vector<StructTypeMember> struct_type_members;
        for (const StructNodeMember& member : members) {
            KPL_ASSERT_THAT(member.identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(member.type_id != TypeId::invalid());
            struct_type_members.push_back({.type_id = member.type_id, .identifier_id = member.identifier_id});
        }
        Type* type = types[struct_type_id.value];
        KPL_ASSERT_THAT(type->type_kind == TypeKind::Struct);
        StructType* struct_type = static_cast<StructType*>(type);
        KPL_ASSERT_THAT(struct_type->members.empty());
        struct_type->members = std::move(struct_type_members);
    }

    Type* TypeTable::lookup(TypeId type_id) {
        KPL_ASSERT_THAT(type_id.value < types.size(), "Type count: {}, received id: {}", types.size(), type_id.value);
        return types[type_id.value];
    }

}
