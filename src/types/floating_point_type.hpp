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

#include <llvm/IR/Value.h>

namespace Kepler::Type {

    class FloatingPointType: public DataType {
    protected:
        llvm::Value* float_to_int_inbounds(llvm::Value* value, TypeToken from, TypeToken to) const;
    };

}
