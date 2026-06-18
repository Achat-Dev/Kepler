// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/type_token.hpp"

#include <llvm/IR/Instructions.h>

namespace Kepler::LocalVariables {

    struct VariableData {
        Type::TypeToken type;
        llvm::AllocaInst* variable;

        VariableData() = default;
        VariableData(Type::TypeToken type, llvm::AllocaInst* alloca) : type(type), variable(alloca) {}
    };

}
