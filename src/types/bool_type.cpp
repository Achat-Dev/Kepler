// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/bool_type.hpp"

#include "compiler.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string BoolType::get_name() const {
        return "bool";
    }

    llvm::Type* BoolType::get_llvm_type() const {
        return llvm::Type::getInt1Ty(Compiler::get_context());
    }

    llvm::Value* BoolType::cast(llvm::Value* value, TypeToken to) const {
        if (to == TypeToken::String) {
            return cast_to_string(value, TypeToken::Bool);
        }
        return nullptr;
    }

    llvm::Value* BoolType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpEQ(lhs, rhs, "bool_eq");
    }

    llvm::Value* BoolType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpNE(lhs, rhs, "bool_neq");
    }

}
