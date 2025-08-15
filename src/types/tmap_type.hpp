// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/data_type.hpp"
#include "types/type_token.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    class TMapType: public DataType {
    public:
        std::string get_name() const override;
        llvm::Type* get_llvm_type() const override;

        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        bool create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data) const override;
        bool create_assign(llvm::Value* value, TypeToken value_type, const LocalVariables::VariableData& variable_data) const override;

        static void create_type();
    };

}
