// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/expression.hpp"
#include "ast/prototype.hpp"

#include <llvm/IR/Function.h>
#include <memory>
#include <vector>

namespace Kepler::AST {

    class Function {
    private:
        std::shared_ptr<Prototype> prototype;
        std::vector<std::unique_ptr<Expression>> body;

    public:
        Function(std::shared_ptr<Prototype> prototype, std::vector<std::unique_ptr<Expression>> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
        llvm::Function* codegen();
    };

}
