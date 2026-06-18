// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/void_type.hpp"

#include "compiler.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string VoidType::get_name() const {
        return "void";
    }

    llvm::Type* VoidType::get_llvm_type() const {
        return llvm::Type::getVoidTy(Compiler::get().get_context());
    }

}
