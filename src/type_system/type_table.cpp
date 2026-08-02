// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_table.hpp"
#include "arena_allocator.hpp"
#include "lexer/operator_type.hpp"
#include "string_pool.hpp"
#include "type_system/type.hpp"
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
        return it->second;
    }

    void TypeTable::create_builtin_types() {
        // Create the types first and fill in the methods afterwards because the methods need to reference the types
        register_builtin_type(&Builtins.unknown_type, TypeKind::Unknown);
        register_builtin_type(&Builtins.void_type, TypeKind::Void);
        existing_types.erase(Builtins.void_type->name_id);
        register_builtin_type(&Builtins.bool_type, TypeKind::Bool);
        register_builtin_type(&Builtins.string_type, TypeKind::String);
        register_builtin_type(&Builtins.i8_type, TypeKind::I8);
        register_builtin_type(&Builtins.i16_type, TypeKind::I16);
        register_builtin_type(&Builtins.i32_type, TypeKind::I32);
        register_builtin_type(&Builtins.i64_type, TypeKind::I64);
        register_builtin_type(&Builtins.f32_type, TypeKind::F32);
        register_builtin_type(&Builtins.f64_type, TypeKind::F64);

        // Finish bool type
        const StringId cast_id = StringPool::get().store("new");
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
        StringId type_name_id = get_type_kind_name_id(type_kind);
        *type_pointer = allocator.allocate<Type>(type_kind, type_name_id, std::vector<Method>{});
        existing_types.emplace(type_name_id, *type_pointer);
    }

    void TypeTable::add_methods_to_builtin_number_type(Type* type) {
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
            {.identifier_id = StringPool::get().store("__math_negate"), .return_type = Builtins.bool_type, .parameter_types = {type}},
        };
        const StringId cast_identifier_id = StringPool::get().store("new");
        for (Type* number_type : number_types) {
            if (type != number_type) {
                common_number_methods.emplace_back(cast_identifier_id, type, std::vector<Type*>{number_type});
            }
        }
        type->methods = std::move(common_number_methods);
    }

}
