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
#include "ast/ast_node.hpp"
#include "ast/ast_pass.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
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

        void create_prototype_symbols(const AbstractSyntaxTree& ast, ModuleId module_id);
        void create_prototype_symbol(ModuleId module_id, Prototype* prototype, LinkageType linkage_type) const;
        void report_unknown_type(StringId type_id, SourceLocation source_location) const;
    };

}
