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
#include "ast/ast_pass.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include <vector>

namespace kepler {

    class ModuleCreationPass : ASTPass<void> {
    public:
        ModuleCreationPass(DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table)
            : diagnostic_sink(diagnostic_sink), symbol_table(symbol_table), type_table(type_table) {}
        void run(std::vector<AbstractSyntaxTree>& asts) override;

    private:
        DiagnosticSink& diagnostic_sink;
        SymbolTable& symbol_table;
        TypeTable& type_table;
    };

}
