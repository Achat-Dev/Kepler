// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/string_pool.hpp"
#include <format>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <string>
#include <string_view>
#include <vector>

namespace kepler {

    enum class TypeKind {
        Unknown,
        Void,
        Bool,
        String,
        I8,
        I16,
        I32,
        I64,
        F32,
        F64,
    };

    struct Type;

    struct Method {
        StringId identifier_id;
        Type* return_type;
        std::vector<Type*> parameter_types;
    };

    struct Type {
        TypeKind type_kind;
        StringId name_id;
        std::vector<Method> methods;

        const Method* find_method(StringId identifier_id, std::vector<Type*> parameter_types) const;
    };

    llvm::Type* get_llvm_type(Type& type, llvm::LLVMContext& context);
    bool is_integer_type(const Type* type);
    bool is_floating_point_type(const Type* type);
    StringId get_type_kind_name_id(TypeKind type_kind);

}

template <>
struct std::formatter<kepler::TypeKind> : std::formatter<std::string> {
    auto format(const kepler::TypeKind& type_kind, std::format_context& ctx) const {
        // A raw switch case would be more efficient, but this avoids duplicating the names
        const kepler::StringId type_name_id = kepler::get_type_kind_name_id(type_kind);
        const std::string_view type_name = kepler::StringPool::get().lookup(type_name_id);
        return std::formatter<std::string>::format(std::format("{}", type_name), ctx);
    }
};

template <>
struct std::formatter<kepler::Type> : std::formatter<std::string> {
    auto format(const kepler::Type& type, std::format_context& ctx) const {
        const std::string_view type_name = kepler::StringPool::get().lookup(type.name_id);
        return std::formatter<std::string>::format(std::format("{}", type_name), ctx);
    }
};
