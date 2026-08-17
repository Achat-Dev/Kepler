// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_table.hpp"
#include "lexer/operator_type.hpp"
#include "type_system/type.hpp"
#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    TypeTable::TypeTable()
        : allocator(kibibyte_size * 16, "type_table") {
        if (Builtins.void_type == nullptr) {
            create_builtin_types();
        }
    }

    Type* TypeTable::lookup(StringId name_id) const {
        const auto it = existing_types.find(name_id);
        if (it == existing_types.end()) {
            return nullptr;
        }
        KPL_ASSERT_NOT_NULLPTR(it->second);
        return it->second;
    }

    void TypeTable::create_builtin_types() {
        KPL_ASSERT_THAT(Builtins.unknown_type == nullptr, "Unknown type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.void_type == nullptr, "Void type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.bool_type == nullptr, "Bool type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.string_type == nullptr, "String type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.i8_type == nullptr, "i8 type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.i16_type == nullptr, "i16 type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.i32_type == nullptr, "i32 type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.i64_type == nullptr, "i64 type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.f32_type == nullptr, "f32 type can't exist when creating the builtin types");
        KPL_ASSERT_THAT(Builtins.f64_type == nullptr, "f64 type can't exist when creating the builtin types");

        // Create the types first and fill in the methods afterwards because the methods need to reference the types
        register_builtin_type(&Builtins.unknown_type, TypeKind::Unknown);
        register_builtin_type(&Builtins.void_type, TypeKind::Void);
        existing_types.emplace(Builtins.void_type->name_id, Builtins.void_type);
        register_builtin_type(&Builtins.bool_type, TypeKind::Bool);
        register_builtin_type(&Builtins.string_type, TypeKind::String);
        register_builtin_type(&Builtins.i8_type, TypeKind::I8);
        register_builtin_type(&Builtins.i16_type, TypeKind::I16);
        register_builtin_type(&Builtins.i32_type, TypeKind::I32);
        register_builtin_type(&Builtins.i64_type, TypeKind::I64);
        register_builtin_type(&Builtins.f32_type, TypeKind::F32);
        register_builtin_type(&Builtins.f64_type, TypeKind::F64);

        // Finish bool type
        const StringId cast_id = StringPool::get().store("__cast");
        const std::vector<Method> bool_methods{
            {
                .identifier_id = get_operator_name_id(OperatorType::Equals),
                .return_type = Builtins.bool_type,
                .parameter_types = {Builtins.bool_type},
            },
            {
                .identifier_id = get_operator_name_id(OperatorType::NotEquals),
                .return_type = Builtins.bool_type,
                .parameter_types = {Builtins.bool_type},
            },
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.i8_type}},
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.i16_type}},
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.i32_type}},
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.i64_type}},
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.f32_type}},
            {.identifier_id = cast_id, .return_type = Builtins.bool_type, .parameter_types = {Builtins.f64_type}},
        };
        Builtins.bool_type->methods = std::move(bool_methods);

        // Finish number types
        add_methods_to_builtin_number_type(Builtins.i8_type);
        add_methods_to_builtin_number_type(Builtins.i16_type);
        add_methods_to_builtin_number_type(Builtins.i32_type);
        add_methods_to_builtin_number_type(Builtins.i64_type);
        add_methods_to_builtin_number_type(Builtins.f32_type);
        add_methods_to_builtin_number_type(Builtins.f64_type);
    }

    void TypeTable::register_builtin_type(Type** type_pointer, TypeKind type_kind) {
        KPL_ASSERT_NOT_NULLPTR(type_pointer);
        KPL_ASSERT_THAT(*type_pointer == nullptr, "Builtin type '{}' can't already exist when registering it", type_kind);
        StringId type_name_id = get_type_kind_name_id(type_kind);
        KPL_ASSERT_THAT(!existing_types.contains(type_name_id), "Builtin type '{}' can't already be present in type map when registering it", type_kind);
        *type_pointer = allocator.allocate<Type>(type_kind, type_name_id, std::vector<Method>{});
        existing_types.emplace(type_name_id, *type_pointer);
    }

    void TypeTable::add_methods_to_builtin_number_type(Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        static const std::vector<Type*> number_types = {
            Builtins.i8_type,
            Builtins.i16_type,
            Builtins.i32_type,
            Builtins.i64_type,
            Builtins.f32_type,
            Builtins.f64_type};
        std::vector<Method> common_number_methods{
            {.identifier_id = get_operator_name_id(OperatorType::Plus), .return_type = type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::Minus), .return_type = type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::Multiplication), .return_type = type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::Division), .return_type = type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::LessThan), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::GreaterThan), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::Equals), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::NotEquals), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::LessEquals), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = get_operator_name_id(OperatorType::GreaterEquals), .return_type = Builtins.bool_type, .parameter_types = {type}},
            {.identifier_id = StringPool::get().store("__math_negate"), .return_type = type, .parameter_types = {type}},
        };
        const StringId cast_id = StringPool::get().store("__cast");
        for (Type* number_type : number_types) {
            if (type != number_type) {
                common_number_methods.emplace_back(cast_id, type, std::vector<Type*>{number_type});
            }
        }
        KPL_ASSERT_THAT(type->methods.empty(), "Methods of builtin number type '{}' must be empty in toder to add the default methods", *type);
        type->methods = std::move(common_number_methods);
    }

}
