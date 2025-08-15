// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/floating_point_type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    class Float64Type: public FloatingPointType {
    public:
        std::string get_name() const override;
        llvm::Type* get_llvm_type() const override;

        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
    };

}
