// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <format>
#include <limits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
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

    llvm::Type* get_llvm_type(const Type* type, llvm::LLVMContext& context) {
        KPL_ASSERT_NOT_NULLPTR(type);

        switch (type->type_kind) {
            case TypeKind::Unknown:
                KPL_ASSERT_UNREACHABLE("Cannot map 'unknown' type to llvm type");
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
            case TypeKind::F32:
                return llvm::Type::getFloatTy(context);
            case TypeKind::F64:
                return llvm::Type::getDoubleTy(context);
        }

        KPL_ASSERT_UNREACHABLE("Missing llvm type mapping for type '{}'", *type);
    }

    bool is_integer_type(const Type* type) {
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

    bool is_floating_point_type(const Type* type) {
        switch (type->type_kind) {
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

        KPL_ASSERT_UNREACHABLE("Missing type name id implementation for type kind '{}'", static_cast<int>(type_kind));
    }

    llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateAdd(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFAdd(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create add implementation for type '{}'", *type);
    }

    llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateSub(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFSub(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create sub implementation for type '{}'", *type);
    }

    llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateMul(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFMul(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create mul implementation for type '{}'", *type);
    }

    llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateSDiv(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFDiv(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create div implementation for type '{}'", *type);
    }

    llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpSLT(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpULT(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create less than implementation for type '{}'", *type);
    }

    llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpSGT(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpUGT(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create greater than implementation for type '{}'", *type);
    }

    llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpEQ(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpUEQ(lhs, rhs);
        } else if (type == TypeTable::Builtins.bool_type) {
            return builder.CreateICmpEQ(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create equals implementation for type '{}'", *type);
    }

    llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpNE(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpUNE(lhs, rhs);
        } else if (type == TypeTable::Builtins.bool_type) {
            return builder.CreateICmpNE(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create not equals implementation for type '{}'", *type);
    }

    llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpSLE(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpULE(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create less equals implementation for type '{}'", *type);
    }

    llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(lhs);
        KPL_ASSERT_NOT_NULLPTR(rhs);
        KPL_ASSERT_NOT_NULLPTR(type);
        if (is_integer_type(type)) {
            return builder.CreateICmpSGE(lhs, rhs);
        } else if (is_floating_point_type(type)) {
            return builder.CreateFCmpUGE(lhs, rhs);
        }
        KPL_ASSERT_UNREACHABLE("Missing create greater equals implementation for type '{}'", *type);
    }

    namespace {
        llvm::Value* create_cast_to_bool(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.bool_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.bool_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                return builder.CreateICmpNE(value, llvm::ConstantInt::get(get_llvm_type(original_type, context), 0));
            } else if (is_floating_point_type(original_type)) {
                return builder.CreateFCmpONE(value, llvm::ConstantFP::get(get_llvm_type(original_type, context), 0));
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to bool implementation for original type '{}'", *original_type);
        }

        template <typename T>
        // clang-format off
        llvm::Value* create_cast_float_to_int(llvm::Value* value,
            const Type* original_type,
            const Type* target_type,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>& builder)
        {
            // clang-format on
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(target_type);
            KPL_ASSERT_THAT(original_type != target_type, "Original type and target type must be different for creating cast from float to int");
            llvm::Type* llvm_float_type = get_llvm_type(original_type, context);
            llvm::Value* min = llvm::ConstantFP::get(llvm_float_type, static_cast<double>(std::numeric_limits<T>::lowest()));
            llvm::Value* max = llvm::ConstantFP::get(llvm_float_type, static_cast<double>(std::numeric_limits<T>::max()));
            llvm::Value* clamped = builder.CreateBinaryIntrinsic(llvm::Intrinsic::maxnum, value, min);
            clamped = builder.CreateBinaryIntrinsic(llvm::Intrinsic::minnum, clamped, max);
            return builder.CreateFPToSI(clamped, get_llvm_type(target_type, context));
        }

        llvm::Value* create_cast_to_i8(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.i8_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.i8_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                return builder.CreateTrunc(value, get_llvm_type(TypeTable::Builtins.i8_type, context));
            } else if (is_floating_point_type(original_type)) {
                return create_cast_float_to_int<int8_t>(value, original_type, TypeTable::Builtins.i8_type, context, builder);
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to i8 implementation for original type '{}'", *original_type);
        }

        llvm::Value* create_cast_to_i16(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.i16_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.i16_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                switch (original_type->type_kind) {
                    case TypeKind::I8:
                        return builder.CreateSExt(value, get_llvm_type(TypeTable::Builtins.i16_type, context));
                    case TypeKind::I32:
                    case TypeKind::I64:
                        return builder.CreateTrunc(value, get_llvm_type(TypeTable::Builtins.i16_type, context));
                    default:
                        KPL_ASSERT_UNREACHABLE("Missing create cast to i16 implementation for integer type '{}'", original_type->type_kind);
                }
            } else if (is_floating_point_type(original_type)) {
                return create_cast_float_to_int<int16_t>(value, original_type, TypeTable::Builtins.i16_type, context, builder);
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to i16 implementation for original type '{}'", *original_type);
        }

        llvm::Value* create_cast_to_i32(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.i32_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.i32_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                switch (original_type->type_kind) {
                    case TypeKind::I8:
                    case TypeKind::I16:
                        return builder.CreateSExt(value, get_llvm_type(TypeTable::Builtins.i32_type, context));
                    case TypeKind::I64:
                        return builder.CreateTrunc(value, get_llvm_type(TypeTable::Builtins.i32_type, context));
                    default:
                        KPL_ASSERT_UNREACHABLE("Missing create cast to i32 implementation for integer type '{}'", original_type->type_kind);
                }
            } else if (is_floating_point_type(original_type)) {
                return create_cast_float_to_int<int32_t>(value, original_type, TypeTable::Builtins.i32_type, context, builder);
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to i32 implementation for original type '{}'", *original_type);
        }

        llvm::Value* create_cast_to_i64(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.i64_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.i64_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                return builder.CreateSExt(value, get_llvm_type(TypeTable::Builtins.i64_type, context));
            } else if (is_floating_point_type(original_type)) {
                return create_cast_float_to_int<int64_t>(value, original_type, TypeTable::Builtins.i64_type, context, builder);
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to i64 implementation for original type '{}'", *original_type);
        }

        llvm::Value* create_cast_to_f32(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.f32_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.f64_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.f32_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                return builder.CreateSIToFP(value, get_llvm_type(original_type, context));
            } else if (original_type == TypeTable::Builtins.f64_type) {
                return builder.CreateFPExt(value, get_llvm_type(original_type, context));
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to f32 implementation for original type '{}'", *original_type);
        }

        llvm::Value* create_cast_to_f64(llvm::Value* value, const Type* original_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.f32_type);
            KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.f64_type);
            KPL_ASSERT_THAT(original_type != TypeTable::Builtins.f64_type, "Can't create redundant cast for type '{}'", *original_type);
            if (is_integer_type(original_type)) {
                return builder.CreateSIToFP(value, get_llvm_type(original_type, context));
            } else if (original_type == TypeTable::Builtins.f32_type) {
                return builder.CreateFPTrunc(value, get_llvm_type(original_type, context));
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to f64 implementation for original type '{}'", *original_type);
        }
    }

    llvm::Value* create_cast(llvm::Value* value, const Type* original_type, const Type* target_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder) {
        KPL_ASSERT_NOT_NULLPTR(value);
        KPL_ASSERT_NOT_NULLPTR(original_type);
        KPL_ASSERT_NOT_NULLPTR(target_type);
        KPL_ASSERT_THAT(original_type != target_type, "Can't create redundant cast for type '{}'", *original_type);
        switch (target_type->type_kind) {
            case TypeKind::Unknown:
            case TypeKind::Void:
            case TypeKind::String:
                KPL_ASSERT_UNREACHABLE("Can't create a cast to target type '{}'", *target_type);

            case TypeKind::Bool:
                return create_cast_to_bool(value, original_type, context, builder);
            case TypeKind::I8:
                return create_cast_to_i8(value, original_type, context, builder);
            case TypeKind::I16:
                return create_cast_to_i16(value, original_type, context, builder);
            case TypeKind::I32:
                return create_cast_to_i32(value, original_type, context, builder);
            case TypeKind::I64:
                return create_cast_to_i64(value, original_type, context, builder);
            case TypeKind::F32:
                return create_cast_to_f32(value, original_type, context, builder);
            case TypeKind::F64:
                return create_cast_to_f64(value, original_type, context, builder);
        }

        KPL_ASSERT_UNREACHABLE("Missing create cast implementation for target type '{}'", *target_type);
    }

}
