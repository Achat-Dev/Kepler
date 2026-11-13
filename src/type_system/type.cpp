// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type.hpp"
#include "ast/ast_node.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler {

    const StructTypeMember* StructType::find_member(StringId identifier_id) const {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        for (const StructTypeMember& member : members) {
            if (member.identifier_id == identifier_id) {
                return &member;
            }
        }
        return nullptr;
    }

    uint32_t StructType::get_member_index(StringId identifier_id) const {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i].identifier_id == identifier_id) {
                return i;
            }
        }
        KPL_ASSERT_UNREACHABLE("Couldn't find an index mapping for struct member '{}'", StringPool::get().lookup(identifier_id));
    }

    StringId get_type_kind_name_id(TypeKind type_kind) {
        switch (type_kind) {
            case TypeKind::Void:
                return StringPool::get().store("void");
            case TypeKind::Bool:
                return StringPool::get().store("bool");
            case TypeKind::String:
                return StringPool::get().store("string");
            case TypeKind::I8:
                return StringPool::get().store("i8");
            case TypeKind::I16:
                return StringPool::get().store("i16");
            case TypeKind::I32:
                return StringPool::get().store("i32");
            case TypeKind::I64:
                return StringPool::get().store("i64");
            case TypeKind::U8:
                return StringPool::get().store("u8");
            case TypeKind::U16:
                return StringPool::get().store("u16");
            case TypeKind::U32:
                return StringPool::get().store("u32");
            case TypeKind::U64:
                return StringPool::get().store("u64");
            case TypeKind::F32:
                return StringPool::get().store("f32");
            case TypeKind::F64:
                return StringPool::get().store("f64");
            case TypeKind::Struct:
                return StringPool::get().store("struct");
        }
        KPL_ASSERT_UNREACHABLE("Missing type name id implementation for type kind '{}'", static_cast<int>(type_kind));
    }

    namespace {
        bool is_initialized = false;
        std::unordered_map<TypeKind, SymbolId> builtin_type_symbol_ids;
        std::unordered_map<TypeKind, TypeId> builtin_type_ids;

        // clang-format off
        void create_builtin_method(TypeId type_id,
            SymbolId type_symbol_id,
            const std::vector<TypeId>& parameter_type_ids,
            const std::vector<StringId>& parameter_identifier_ids,
            StringId type_identifier_id,
            OperatorType operator_type,
            SymbolTable& symbol_table)
        {
            // clang-format on
            const ModuleId global_module_id = symbol_table.get_global_module()->id;
            // Lookup the operator identifier first, because that one might store a new string,
            // which could invalidate the type identifier if that is looked up first
            const std::string& operator_identifier = StringPool::get().lookup(get_operator_name_id(operator_type));
            const std::string& type_identifier = StringPool::get().lookup(type_identifier_id);
            const StringId method_identifier_id = StringPool::get().store(type_identifier + '.' + operator_identifier);
            const auto symbol_id = symbol_table.create_prototype(global_module_id, type_id, method_identifier_id,
                LinkageType::Export, parameter_type_ids, parameter_identifier_ids, false, type_symbol_id, true);
            KPL_ASSERT_THAT(symbol_id.has_value());
        }

        void create_builtin_bool_type_methods(SymbolTable& symbol_table) {
            const ModuleId global_module_id = symbol_table.get_global_module()->id;
            const SymbolId type_symbol_id = builtin_type_symbol_ids[TypeKind::Bool];
            const TypeId bool_type_id = builtin_type_ids[TypeKind::Bool];

            // Create operator methods
            const StringId a_identifier_id = StringPool::get().store("a");
            const StringId b_identifier_id = StringPool::get().store("b");
            const StringId bool_identifier_id = get_type_kind_name_id(TypeKind::Bool);
            const std::vector<TypeId> parameter_type_ids{bool_type_id, bool_type_id};
            const std::vector<StringId> parameter_identifier_ids{a_identifier_id, b_identifier_id};
            create_builtin_method(bool_type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                bool_identifier_id, OperatorType::Equals, symbol_table);
            create_builtin_method(bool_type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                bool_identifier_id, OperatorType::NotEquals, symbol_table);

            // Create cast methods
            const StringId value_identifier_id = StringPool::get().store("value");
            const StringId cast_identifier_id = StringPool::get().store(StringPool::get().lookup(bool_identifier_id) + ".__cast");
            const std::vector<TypeId> number_type_ids{
                builtin_type_ids[TypeKind::I8],
                builtin_type_ids[TypeKind::I16],
                builtin_type_ids[TypeKind::I32],
                builtin_type_ids[TypeKind::I64],
                builtin_type_ids[TypeKind::U8],
                builtin_type_ids[TypeKind::U16],
                builtin_type_ids[TypeKind::U32],
                builtin_type_ids[TypeKind::U64],
                builtin_type_ids[TypeKind::F32],
                builtin_type_ids[TypeKind::F64],
            };
            for (TypeId number_type_id : number_type_ids) {
                const auto cast_symbol_id = symbol_table.create_prototype(global_module_id, bool_type_id, cast_identifier_id,
                    LinkageType::Export, {number_type_id}, {value_identifier_id}, false, type_symbol_id, true);
            }
        }

        void create_builtin_number_type_methods(TypeKind type_kind, SymbolTable& symbol_table) {
            KPL_ASSERT_THAT(type_kind != TypeKind::Struct);
            const SymbolId type_symbol_id = builtin_type_symbol_ids[type_kind];
            const TypeSymbol* type_symbol = static_cast<TypeSymbol*>(symbol_table.lookup(type_symbol_id));
            const ModuleId global_module_id = symbol_table.get_global_module()->id;
            const TypeId type_id = type_symbol->type_id;
            const StringId type_identifier_id = get_type_kind_name_id(type_kind);

            // Create operator methods
            const StringId a_identifier_id = StringPool::get().store("a");
            const StringId b_identifier_id = StringPool::get().store("b");
            const StringId value_identifier_id = StringPool::get().store("value");
            const std::vector<TypeId> parameter_type_ids{type_id, type_id};
            const std::vector<StringId> parameter_identifier_ids{a_identifier_id, b_identifier_id};
            // Mathematical operators
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::Plus, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::Minus, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::Multiplication, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::Division, symbol_table);
            const StringId mneg_identifier_id = StringPool::get().store(StringPool::get().lookup(type_identifier_id) + ".__mneg");
            const auto mneg_symbol_id = symbol_table.create_prototype(global_module_id, type_id, mneg_identifier_id,
                LinkageType::Export, {type_id}, {value_identifier_id}, false, type_symbol_id, true);
            KPL_ASSERT_THAT(mneg_symbol_id.has_value());

            // Comparison operators
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::LessThan, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::GreaterThan, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::Equals, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::NotEquals, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::LessEquals, symbol_table);
            create_builtin_method(type_id, type_symbol_id, parameter_type_ids, parameter_identifier_ids,
                type_identifier_id, OperatorType::GreaterEquals, symbol_table);

            // Create cast methods
            const StringId cast_identifier_id = StringPool::get().store(StringPool::get().lookup(type_identifier_id) + ".__cast");
            std::vector<TypeId> number_type_ids = {
                builtin_type_ids[TypeKind::I8],
                builtin_type_ids[TypeKind::I16],
                builtin_type_ids[TypeKind::I32],
                builtin_type_ids[TypeKind::I64],
                builtin_type_ids[TypeKind::U8],
                builtin_type_ids[TypeKind::U16],
                builtin_type_ids[TypeKind::U32],
                builtin_type_ids[TypeKind::U64],
                builtin_type_ids[TypeKind::F32],
                builtin_type_ids[TypeKind::F64],
            };
            for (TypeId number_type_id : number_type_ids) {
                if (number_type_id == type_id) {
                    continue;
                }
                const auto cast_symbol_id = symbol_table.create_prototype(global_module_id, type_id, cast_identifier_id,
                    LinkageType::Export, {number_type_id}, {value_identifier_id}, false, type_symbol_id, true);
                KPL_ASSERT_THAT(cast_symbol_id.has_value());
            }
        }

        SymbolId create_builtin_type(TypeKind type_kind, SymbolTable& symbol_table, TypeTable& type_table) {
            KPL_ASSERT_THAT(type_kind != TypeKind::Struct);
            const ModuleId global_module_id = symbol_table.get_global_module()->id;
            const StringId identifier_id = get_type_kind_name_id(type_kind);
            const auto symbol_id = symbol_table.create_struct(global_module_id, identifier_id, true);
            KPL_ASSERT_THAT(symbol_id.has_value());
            const TypeId type_id = type_table.create_type(identifier_id, type_kind, symbol_id.value());
            symbol_table.lookup(symbol_id.value())->type_id = type_id;
            builtin_type_ids.emplace(type_kind, type_id);
            builtin_type_symbol_ids.emplace(type_kind, symbol_id.value());
            return symbol_id.value();
        }
    }

    void create_builtin_types(SymbolTable& symbol_table, TypeTable& type_table) {
        KPL_ASSERT_THAT(!is_initialized);
        // Create builtin types and type symbols
        create_builtin_type(TypeKind::Void, symbol_table, type_table);
        create_builtin_type(TypeKind::Bool, symbol_table, type_table);
        create_builtin_type(TypeKind::String, symbol_table, type_table);
        create_builtin_type(TypeKind::I8, symbol_table, type_table);
        create_builtin_type(TypeKind::I16, symbol_table, type_table);
        create_builtin_type(TypeKind::I32, symbol_table, type_table);
        create_builtin_type(TypeKind::I64, symbol_table, type_table);
        create_builtin_type(TypeKind::U8, symbol_table, type_table);
        create_builtin_type(TypeKind::U16, symbol_table, type_table);
        create_builtin_type(TypeKind::U32, symbol_table, type_table);
        create_builtin_type(TypeKind::U64, symbol_table, type_table);
        create_builtin_type(TypeKind::F32, symbol_table, type_table);
        create_builtin_type(TypeKind::F64, symbol_table, type_table);

        // Create builtin type methods
        create_builtin_bool_type_methods(symbol_table);
        create_builtin_number_type_methods(TypeKind::I8, symbol_table);
        create_builtin_number_type_methods(TypeKind::I16, symbol_table);
        create_builtin_number_type_methods(TypeKind::I32, symbol_table);
        create_builtin_number_type_methods(TypeKind::I64, symbol_table);
        create_builtin_number_type_methods(TypeKind::U8, symbol_table);
        create_builtin_number_type_methods(TypeKind::U16, symbol_table);
        create_builtin_number_type_methods(TypeKind::U32, symbol_table);
        create_builtin_number_type_methods(TypeKind::U64, symbol_table);
        create_builtin_number_type_methods(TypeKind::F32, symbol_table);
        create_builtin_number_type_methods(TypeKind::F64, symbol_table);

        is_initialized = true;
    }

    SymbolId get_builtin_type_symbol_id(TypeKind type_kind) {
        KPL_ASSERT_THAT(is_initialized);
        KPL_ASSERT_THAT(builtin_type_symbol_ids.contains(type_kind), "Builtin type symbols dont't contain type kind {}", type_kind);
        return builtin_type_symbol_ids[type_kind];
    }

    TypeId get_builtin_type_id(TypeKind type_kind) {
        KPL_ASSERT_THAT(is_initialized);
        KPL_ASSERT_THAT(builtin_type_symbol_ids.contains(type_kind), "Builtin types dont't contain type kind {}", type_kind);
        return builtin_type_ids[type_kind];
    }

    llvm::Type* get_builtin_llvm_type(TypeKind type_kind, llvm::LLVMContext& context) {
        KPL_ASSERT_THAT(is_initialized);
        switch (type_kind) {
            case TypeKind::Void:
                return llvm::Type::getVoidTy(context);
            case TypeKind::Bool:
                return llvm::Type::getInt1Ty(context);
            case TypeKind::String:
                // A string is internally represented as an immutable array of i8
                // However, to get the llvm::Type* of that, the length of the array is needed
                // That's why the type of a string is an i8* (since llvm uses opaque pointers, the pointer is not explicitly typed)
                return llvm::PointerType::get(context, 0);
            case TypeKind::I8:
                return llvm::Type::getInt8Ty(context);
            case TypeKind::I16:
                return llvm::Type::getInt16Ty(context);
            case TypeKind::I32:
                return llvm::Type::getInt32Ty(context);
            case TypeKind::I64:
                return llvm::Type::getInt64Ty(context);
            case TypeKind::U8:
                return llvm::Type::getInt8Ty(context);
            case TypeKind::U16:
                return llvm::Type::getInt16Ty(context);
            case TypeKind::U32:
                return llvm::Type::getInt32Ty(context);
            case TypeKind::U64:
                return llvm::Type::getInt64Ty(context);
            case TypeKind::F32:
                return llvm::Type::getFloatTy(context);
            case TypeKind::F64:
                return llvm::Type::getDoubleTy(context);
            case TypeKind::Struct:
                KPL_ASSERT_UNREACHABLE("Cannot map type kind '{}' to llvm type", TypeKind::Struct);
        }
        KPL_ASSERT_UNREACHABLE("Missing llvm type mapping for type kind {}", static_cast<int>(type_kind));
    }

    bool is_builtin_type(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        switch (type->type_kind) {
            case TypeKind::Void:
            case TypeKind::Bool:
            case TypeKind::String:
            case TypeKind::I8:
            case TypeKind::I16:
            case TypeKind::I32:
            case TypeKind::I64:
            case TypeKind::U8:
            case TypeKind::U16:
            case TypeKind::U32:
            case TypeKind::U64:
            case TypeKind::F32:
            case TypeKind::F64:
                return true;
            case TypeKind::Struct:
                return false;
        }
        KPL_ASSERT_UNREACHABLE("Missing type name id implementation for type kind '{}'", static_cast<int>(type->type_kind));
    }

    bool is_integer_type(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        switch (type->type_kind) {
            case TypeKind::I8:
            case TypeKind::I16:
            case TypeKind::I32:
            case TypeKind::I64:
            case TypeKind::U8:
            case TypeKind::U16:
            case TypeKind::U32:
            case TypeKind::U64:
                return true;
            default:
                return false;
        }
    }

    bool is_signed_integer_type(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        switch (type->type_kind) {
            case TypeKind::I8:
            case TypeKind::I16:
            case TypeKind::I32:
            case TypeKind::I64:
                return true;
            default:
                return false;
        }
    }

    bool is_unsigned_integer_type(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        switch (type->type_kind) {
            case TypeKind::U8:
            case TypeKind::U16:
            case TypeKind::U32:
            case TypeKind::U64:
                return true;
            default:
                return false;
        }
    }

    bool is_floating_point_type(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        switch (type->type_kind) {
            case TypeKind::F32:
            case TypeKind::F64:
                return true;
            default:
                return false;
        }
    }

    uint32_t get_integer_bitwidth(const Type* type) {
        KPL_ASSERT_NOT_NULLPTR(type);
        KPL_ASSERT_THAT(is_integer_type(type), "Required integer type, received: {}", *type);
        switch (type->type_kind) {
            case kepler::TypeKind::I8:
                return 8;
            case kepler::TypeKind::I16:
                return 16;
            case kepler::TypeKind::I32:
                return 32;
            case kepler::TypeKind::I64:
                return 64;
            case kepler::TypeKind::U8:
                return 8;
            case kepler::TypeKind::U16:
                return 16;
            case kepler::TypeKind::U32:
                return 32;
            case kepler::TypeKind::U64:
                return 64;
            default:
                break;
        }
        KPL_ASSERT_UNREACHABLE("Missing integer bit width implementation for type '{}'", *type);
    }

}
