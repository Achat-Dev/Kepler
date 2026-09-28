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
#include "lexer/operator_type.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    TypeTable::TypeTable(SymbolTable& symbol_table)
        : allocator(kibibyte_size * 16, "type_table") {
        if (Builtins.void_type_id == TypeId::invalid()) {
            create_builtin_types(symbol_table);
        }
    }

    TypeId TypeTable::create_struct(StringId identifier_id) {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        const TypeId type_id{.value = static_cast<uint32_t>(types.size())};
        std::vector<StructTypeMember> struct_type_members;
        types.push_back(allocator.allocate<StructType>(type_id, TypeKind::Struct, identifier_id, std::vector<Method>{}, std::vector<StructTypeMember>{}));
        return type_id;
    }

    void TypeTable::create_struct_members(TypeId struct_type_id, const std::vector<StructMemberData>& members) {
        KPL_ASSERT_THAT(struct_type_id.value < types.size(), "Type count: {}, received id: {}", types.size(), struct_type_id.value);
        KPL_ASSERT_THAT(struct_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(struct_type_id != Builtins.unknown_type_id);
        std::vector<StructTypeMember> struct_type_members;
        for (const StructMemberData& member_data : members) {
            KPL_ASSERT_THAT(member_data.identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(member_data.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(member_data.type_id != Builtins.unknown_type_id);
            struct_type_members.push_back({.type_id = member_data.type_id, .identifier_id = member_data.identifier_id});
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

    void TypeTable::create_builtin_types(SymbolTable& symbol_table) {
        KPL_ASSERT_THAT(Builtins.unknown_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.void_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.bool_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.string_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.i8_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.i16_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.i32_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.i64_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.u8_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.u16_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.u32_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.u64_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.f32_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(Builtins.f64_type_id == TypeId::invalid());

        // Create the types first and fill in the methods afterwards because the methods need to reference the types
        create_builtin_type(Builtins.unknown_type_id, TypeKind::Unknown);
        create_builtin_type(Builtins.void_type_id, TypeKind::Void);
        create_builtin_type(Builtins.bool_type_id, TypeKind::Bool);
        create_builtin_type(Builtins.string_type_id, TypeKind::String);
        create_builtin_type(Builtins.i8_type_id, TypeKind::I8);
        create_builtin_type(Builtins.i16_type_id, TypeKind::I16);
        create_builtin_type(Builtins.i32_type_id, TypeKind::I32);
        create_builtin_type(Builtins.i64_type_id, TypeKind::I64);
        create_builtin_type(Builtins.u8_type_id, TypeKind::U8);
        create_builtin_type(Builtins.u16_type_id, TypeKind::U16);
        create_builtin_type(Builtins.u32_type_id, TypeKind::U32);
        create_builtin_type(Builtins.u64_type_id, TypeKind::U64);
        create_builtin_type(Builtins.f32_type_id, TypeKind::F32);
        create_builtin_type(Builtins.f64_type_id, TypeKind::F64);

        // Finish bool type
        const StringId cast_id = StringPool::get().store("__cast");
        const std::vector<Method> bool_methods{
            {
                .identifier_id = get_operator_name_id(OperatorType::Equals),
                .return_type_id = Builtins.bool_type_id,
                .parameter_type_ids = {Builtins.bool_type_id},
            },
            {
                .identifier_id = get_operator_name_id(OperatorType::NotEquals),
                .return_type_id = Builtins.bool_type_id,
                .parameter_type_ids = {Builtins.bool_type_id},
            },
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.i8_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.i16_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.i32_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.i64_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.u8_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.u16_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.u32_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.u64_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.f32_type_id}},
            {.identifier_id = cast_id, .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {Builtins.f64_type_id}},
        };
        types[Builtins.bool_type_id.value]->methods = std::move(bool_methods);

        // Finish number types
        add_methods_to_builtin_number_type(Builtins.i8_type_id);
        add_methods_to_builtin_number_type(Builtins.i16_type_id);
        add_methods_to_builtin_number_type(Builtins.i32_type_id);
        add_methods_to_builtin_number_type(Builtins.i64_type_id);
        add_methods_to_builtin_number_type(Builtins.u8_type_id);
        add_methods_to_builtin_number_type(Builtins.u16_type_id);
        add_methods_to_builtin_number_type(Builtins.u32_type_id);
        add_methods_to_builtin_number_type(Builtins.u64_type_id);
        add_methods_to_builtin_number_type(Builtins.f32_type_id);
        add_methods_to_builtin_number_type(Builtins.f64_type_id);
    }

    void TypeTable::create_builtin_type(TypeId& type_id, TypeKind type_kind) {
        KPL_ASSERT_THAT(type_id == TypeId::invalid());
        type_id.value = static_cast<uint32_t>(types.size());
        StringId type_name_id = get_type_kind_name_id(type_kind);
        types.push_back(allocator.allocate<Type>(type_id, type_kind, type_name_id));
    }

    void TypeTable::add_methods_to_builtin_number_type(TypeId type_id) {
        KPL_ASSERT_THAT(type_id.value < types.size(), "Type count: {}, received id: {}", types.size(), type_id.value);
        static const std::vector<TypeId> number_type_ids = {
            Builtins.i8_type_id,
            Builtins.i16_type_id,
            Builtins.i32_type_id,
            Builtins.i64_type_id,
            Builtins.u8_type_id,
            Builtins.u16_type_id,
            Builtins.u32_type_id,
            Builtins.u64_type_id,
            Builtins.f32_type_id,
            Builtins.f64_type_id};
        std::vector<Method> common_number_methods{
            {.identifier_id = get_operator_name_id(OperatorType::Plus), .return_type_id = type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::Minus), .return_type_id = type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::Multiplication), .return_type_id = type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::Division), .return_type_id = type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::LessThan), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::GreaterThan), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::Equals), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::NotEquals), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::LessEquals), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = get_operator_name_id(OperatorType::GreaterEquals), .return_type_id = Builtins.bool_type_id, .parameter_type_ids = {type_id}},
            {.identifier_id = StringPool::get().store("__math_negate"), .return_type_id = type_id, .parameter_type_ids = {type_id}},
        };
        const StringId cast_id = StringPool::get().store("__cast");
        for (TypeId number_type : number_type_ids) {
            if (type_id != number_type) {
                common_number_methods.emplace_back(cast_id, type_id, std::vector<TypeId>{number_type});
            }
        }
        Type* type = types[type_id.value];
        KPL_ASSERT_THAT(type->methods.empty());
        type->methods = std::move(common_number_methods);
    }

}
