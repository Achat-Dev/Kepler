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
#include "diagnostics/diagnostic_sink.hpp"

namespace kepler {

    // The virtual function call could be eliminated by using CRTP
    template <typename T>
    class ASTPass {
    public:
        ASTPass(const AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink)
            : ast(ast), diagnostic_sink(diagnostic_sink) {}
        virtual ~ASTPass() = default;
        virtual T run() = 0;

    protected:
        const AbstractSyntaxTree& ast;
        DiagnosticSink& diagnostic_sink;
    };

}
