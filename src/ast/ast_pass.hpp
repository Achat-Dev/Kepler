// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"

namespace kepler {

    // The virtual function call could be eliminated by using CRTP
    template <typename T>
    class ASTPass {
    public:
        explicit ASTPass(AbstractSyntaxTree& ast) : ast(ast) {}
        virtual ~ASTPass() = default;
        virtual T run() = 0;

    protected:
        AbstractSyntaxTree& ast;
    };

}
