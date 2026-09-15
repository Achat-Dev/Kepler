// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/module_creation_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <vector>

namespace kepler {

    void ModuleCreationPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_THAT(ast.module_definition.id == ModuleId::invalid());
            ast.module_definition.id = symbol_table.create_module(ast.module_definition);
            create_prototype_symbols(ast, ast.module_definition.id);
        }
    }

    void ModuleCreationPass::create_prototype_symbols(const AbstractSyntaxTree& ast, ModuleId module_id) {
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    const Extern* ext = static_cast<Extern*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(ext->prototype);
                    create_prototype_symbol(ext->prototype.get(), module_id);
                    break;
                }
                case ASTNodeType::Function: {
                    const Function* function = static_cast<Function*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(function->prototype);
                    create_prototype_symbol(function->prototype.get(), module_id);
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type of type '{}' on top level during name resolution", node->node_type);
            }
        }
    }

    void ModuleCreationPass::create_prototype_symbol(Prototype* prototype, ModuleId module_id) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        Type* return_type = type_table.lookup(prototype->return_type_id);
        if (return_type == nullptr) {
            report_unknown_type(prototype->return_type_id, prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return;
        } else {
            KPL_ASSERT_THAT(prototype->return_type == nullptr);
            prototype->return_type = return_type;
        }

        std::vector<Type*> parameter_types;
        parameter_types.reserve(prototype->parameter_data.size());
        for (auto& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type == nullptr);
            Type* parameter_type = type_table.lookup(parameter_data.type_id);
            if (parameter_type == nullptr) {
                report_unknown_type(parameter_data.type_id, parameter_data.type_source_location);
                prototype->node_type = ASTNodeType::Poison;
                return;
            }
            parameter_data.type = parameter_type;
            parameter_types.push_back(parameter_type);
        }

        const auto symbol = symbol_table.create_prototype(module_id,
            return_type,
            prototype->identifier_id,
            prototype->linkage_type,
            std::move(parameter_types),
            prototype->is_variadic,
            prototype->identifier_source_location);
        if (!symbol) {
            const SourceDiagnostic& diagnostic = symbol.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, diagnostic.source_location);
            prototype->node_type = ASTNodeType::Poison;
        } else {
            prototype->symbol_id = *symbol;
        }
    }

    void ModuleCreationPass::report_unknown_type(StringId type_id, SourceLocation source_location) const {
        const std::string_view type_name = StringPool::get().lookup(type_id);
        diagnostic_sink.report(DiagnosticCode::UnknownType, std::format("Unknown type '{}'", type_name), source_location);
    }

}
