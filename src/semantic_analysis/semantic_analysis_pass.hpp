// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_pass.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "semantic_analysis/symbol_table.hpp"

namespace kepler {

    class SemanticAnalysisPass : public ASTPass {
    public:
        SemanticAnalysisPass(const AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table)
            : ASTPass(ast, diagnostic_sink), symbol_table(symbol_table) {}
        void run() const override;

    private:
        SymbolTable& symbol_table;

        void collect_prototype_symbols() const;
        void create_prototype_symbol(const Prototype* prototype) const;
    };

}
