// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/semantic_analysis_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic.hpp"
#include "emergency.hpp"
#include "type_system/data_type_kind.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace kepler::semantic_analysis {

    void SemanticAnalysisPass::run() const {
        collect_prototype_symbols();

        // Analyse function contents
        for (const auto& ast_node : ast.nodes) {
        }
    }

    void SemanticAnalysisPass::collect_prototype_symbols() const {
        for (const auto& ast_node : ast.nodes) {
            switch (ast_node->node_type) {
                case ast::ASTNodeType::Function: {
                    const ast::Function* function = static_cast<ast::Function*>(ast_node.get());
                    create_prototype_symbol(function->prototype.get());
                    break;
                }
                case ast::ASTNodeType::Prototype: {
                    const ast::Prototype* prototype = static_cast<ast::Prototype*>(ast_node.get());
                    create_prototype_symbol(prototype);
                    break;
                }
                default:
                    emergency_exit("Behold: I somehow managed to create a malformed AST with a node of type '{}' on the top level <(˘ ˘ ˘)>", ast_node->node_type);
                    break;
            }
        }
    }

    void SemanticAnalysisPass::create_prototype_symbol(const ast::Prototype* prototype) const {
        std::vector<type_system::DataTypeKind> parameter_data_types;
        parameter_data_types.reserve(prototype->parameter_data.size());
        for (const auto& parameter_data : prototype->parameter_data) {
            parameter_data_types.push_back(parameter_data.data_type);
        }
        const auto& symbol = symbol_table.create_prototype(prototype->return_type, prototype->identifier, prototype->linkage_type, std::move(parameter_data_types), prototype->source_location);
        if (!symbol) {
            const diagnostics::SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
        }
    }

}
