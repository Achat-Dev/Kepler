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
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/string_pool.hpp"
#include <vector>

namespace kepler {

    class TypeTable {
    public:
        TypeTable() : allocator(kibibyte_size * 16, "types") {}
        TypeId create_type(StringId identifier_id, TypeKind type_kind, SymbolId symbol_id);
        TypeId create_struct(StringId identifier_id, SymbolId symbol_id);
        void create_struct_members(TypeId struct_type_id, const std::vector<StructNodeMember>& members);
        Type* lookup(TypeId type_id);

    private:
        std::vector<Type*> types;
        ArenaAllocator allocator;
    };

}
