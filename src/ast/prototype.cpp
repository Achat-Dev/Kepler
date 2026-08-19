// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/prototype.hpp"
#include "utils/assert.h"
#include <format>
#include <llvm/IR/Function.h>

namespace kepler {

    llvm::Function::LinkageTypes get_llvm_linkage_type(Prototype::LinkageType linkage_type) {
        switch (linkage_type) {
            case Prototype::LinkageType::Internal:
                return llvm::Function::InternalLinkage;
            case Prototype::LinkageType::External:
                return llvm::Function::ExternalLinkage;
        }

        KPL_ASSERT_UNREACHABLE("Missing llvm linkage type mapping for linkage type '{}'", static_cast<int>(linkage_type));
    }

}
