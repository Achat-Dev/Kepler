// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/float32_type.hpp"

#include "compiler.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string Float32Type::get_name() const {
        return "f32";
    }

    llvm::Type* Float32Type::get_llvm_type() const {
        return llvm::Type::getFloatTy(Compiler::get_context());
    }

    llvm::Value* Float32Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float32), 0));
            case TypeToken::String: return cast_to_string(value, TypeToken::Float32);
            case TypeToken::Int8: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int16: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int32: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int64: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Float64: return Compiler::get_builder().CreateFPExt(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Float32Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFAdd(lhs, rhs, "f32_add");
    }

    llvm::Value* Float32Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFSub(lhs, rhs, "f32_sub");
    }

    llvm::Value* Float32Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFMul(lhs, rhs, "f32_mul");
    }

    llvm::Value* Float32Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFDiv(lhs, rhs, "f32_div");
    }

    llvm::Value* Float32Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpULT(lhs, rhs, "f32_lt");
    }

    llvm::Value* Float32Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUGT(lhs, rhs, "f32_gt");
    }

    llvm::Value* Float32Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUEQ(lhs, rhs, "f32_eq");
    }

    llvm::Value* Float32Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUNE(lhs, rhs, "f32_neq");
    }

    llvm::Value* Float32Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpULE(lhs, rhs, "f32_leq");
    }

    llvm::Value* Float32Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUGE(lhs, rhs, "f32_geq");
    }

}
