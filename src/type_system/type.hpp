// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <limits>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

namespace kepler {

    enum class TypeKind {
        Void,
        Bool,
        String,
        I8,
        I16,
        I32,
        I64,
        U8,
        U16,
        U32,
        U64,
        F32,
        F64,
        Struct,
    };

    struct TypeId {
        uint32_t value = std::numeric_limits<uint32_t>::max();

        bool operator==(const TypeId& other) const = default;
        bool operator!=(const TypeId& other) const = default;
        static constexpr TypeId invalid() { return TypeId{}; }
    };

    /*struct Method {
        TypeId return_type_id;
        StringId identifier_id;
        std::vector<TypeId> parameter_type_ids;
        SymbolId prototype_symbol_id;
        bool is_variadic = false;
    };*/

    // struct SymbolTable;
    // struct TypeTable;

    struct Type {
        TypeId id;
        TypeKind type_kind;
        StringId identifier_id;
        SymbolId symbol_id;
        /*std::vector<Method> methods;

        void register_method(SymbolId prototype_symbol_id, SymbolTable* symbol_table, TypeTable* type_table);
        const Method* find_method(TypeId return_type_id,
            StringId identifier_id,
            const std::vector<TypeId>& parameter_type_ids,
            bool is_variadic,
            SymbolTable* symbol_table) const;*/
    };

    struct StructTypeMember {
        TypeId type_id;
        StringId identifier_id;
    };

    struct StructType : Type {
        std::vector<StructTypeMember> members;

        const StructTypeMember* find_member(StringId identifier_id) const;
        uint32_t get_member_index(StringId identifier_id) const;
    };

    StringId get_type_kind_name_id(TypeKind type_kind);
    bool is_builtin_type(const Type* type);
    bool is_integer_type(const Type* type);
    bool is_signed_integer_type(const Type* type);
    bool is_unsigned_integer_type(const Type* type);
    bool is_floating_point_type(const Type* type);
    uint32_t get_integer_bitwidth(const Type* type);

    llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type, llvm::IRBuilder<>& builder);
    llvm::Value* create_cast(llvm::Value* value, const Type* original_type, const Type* target_type, llvm::LLVMContext& context, llvm::IRBuilder<>& builder);

}

template <>
struct std::formatter<kepler::TypeKind> : std::formatter<std::string> {
    auto format(const kepler::TypeKind& type_kind, std::format_context& ctx) const {
        // A raw switch case would be more efficient, but this avoids duplicating the names
        const kepler::StringId type_name_id = kepler::get_type_kind_name_id(type_kind);
        const std::string& type_name = kepler::StringPool::get().lookup(type_name_id);
        return std::formatter<std::string>::format(std::format("{}", type_name), ctx);
    }
};

template <>
struct std::formatter<kepler::Type> : std::formatter<std::string> {
    auto format(const kepler::Type& type, std::format_context& ctx) const {
        const std::string& type_name = kepler::StringPool::get().lookup(type.identifier_id);
        return std::formatter<std::string>::format(std::format("{}", type_name), ctx);
    }
};

template <>
struct std::hash<kepler::TypeId> {
    size_t operator()(const kepler::TypeId& id) const noexcept {
        return hash<uint32_t>{}(id.value);
    }
};
