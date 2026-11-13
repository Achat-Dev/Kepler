// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/symbol_id.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
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

    struct Type {
        TypeId id;
        TypeKind type_kind;
        StringId identifier_id;
        SymbolId symbol_id;
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

    struct SymbolTable;
    struct TypeTable;

    void create_builtin_types(SymbolTable& symbol_table, TypeTable& type_table);
    StringId get_type_kind_name_id(TypeKind type_kind);
    SymbolId get_builtin_type_symbol_id(TypeKind type_kind);
    TypeId get_builtin_type_id(TypeKind type_kind);
    llvm::Type* get_builtin_llvm_type(TypeKind type_kind, llvm::LLVMContext& context);
    bool is_builtin_type(const Type* type);
    bool is_integer_type(const Type* type);
    bool is_signed_integer_type(const Type* type);
    bool is_unsigned_integer_type(const Type* type);
    bool is_floating_point_type(const Type* type);
    uint32_t get_integer_bitwidth(const Type* type);

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
