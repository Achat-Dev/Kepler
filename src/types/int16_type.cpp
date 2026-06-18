// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/int16_type.hpp"

#include "compiler.hpp"
#include "log.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string Int16Type::get_name() const {
        return "i16";
    }

    llvm::Type* Int16Type::get_llvm_type() const {
        return llvm::Type::getInt16Ty(Compiler::get().get_context());
    }

    llvm::Value* Int16Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get().get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int16), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String: return cast_to_string(value, TypeToken::Int16);
            case TypeToken::Int8: return Compiler::get().get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int32: return Compiler::get().get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get().get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get().get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get().get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Int16Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateAdd(lhs, rhs, "i16_add");
    }

    llvm::Value* Int16Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateSub(lhs, rhs, "i16_sub");
    }

    llvm::Value* Int16Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateMul(lhs, rhs, "i16_mul");
    }

    llvm::Value* Int16Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateSDiv(lhs, rhs, "i16_div");
    }

    llvm::Value* Int16Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpSLT(lhs, rhs, "i16_lt");
    }

    llvm::Value* Int16Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpSGT(lhs, rhs, "i16_gt");
    }

    llvm::Value* Int16Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpEQ(lhs, rhs, "i16_eq");
    }

    llvm::Value* Int16Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpNE(lhs, rhs, "i16_neq");
    }

    llvm::Value* Int16Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpSLE(lhs, rhs, "i16_leq");
    }

    llvm::Value* Int16Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get().get_builder().CreateICmpSGE(lhs, rhs, "i16_geq");
    }

}
