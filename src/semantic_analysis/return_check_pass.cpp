// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/return_check_pass.hpp"
#include "ast/ast_node.hpp"
#include "ast/function.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    void ReturnCheckPass::run() {
        for (const std::unique_ptr<ASTNode>& node : ast.top_level_nodes) {
            KPL_ASSERT_THAT(node->node_type != ASTNodeType::Poison, "All top level nodes must be unpoisoned for return checking");
            switch (node->node_type) {
                case ASTNodeType::Function: {
                    Function* function = static_cast<Function*>(node.get());
                    KPL_ASSERT_THAT(function->body.contains_return == false,
                        "Flag whether a function body contains a 'return' statement must be false for return checking");
                    const ReturnCheckResult return_result = check_body(function->body.nodes, ReturnCheckBodyType::FunctionBody);
                    if (return_result.contains_return) {
                        function->body.contains_return = true;
                    } else {
                        KPL_ASSERT_NOT_NULLPTR(function->prototype);
                        KPL_ASSERT_NOT_NULLPTR(TypeTable::Builtins.void_type);
                        if (function->prototype->return_type_id != TypeTable::Builtins.void_type->name_id) {
                            const std::string_view identifier = StringPool::get().lookup(function->prototype->identifier_id);
                            const std::string message = std::format("Not all code paths of function '{}' contain a return statement", identifier);
                            diagnostic_sink.report(DiagnosticCode::MissingReturn, std::move(message), function->source_location);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    ReturnCheckResult ReturnCheckPass::check_body(const std::vector<std::unique_ptr<ASTNode>>& nodes, ReturnCheckBodyType body_type) {
        for (size_t i = 0; i < nodes.size(); i++) {
            KPL_ASSERT_THAT(nodes[i]->node_type != ASTNodeType::Poison, "All body nodes must be unpoisoned for return checking");
            switch (nodes[i]->node_type) {
                case ASTNodeType::IfStatement: {
                    IfStatement* if_statement = static_cast<IfStatement*>(nodes[i].get());
                    KPL_ASSERT_THAT(if_statement->if_body.contains_return == false,
                        "Flag whether an if body contains a 'return' statement must be false for return checking");
                    KPL_ASSERT_THAT(if_statement->else_body.contains_return == false,
                        "Flag whether an if body contains a 'return' statement must be false for return checking");
                    const ReturnCheckResult if_body_rcr = check_body(if_statement->if_body.nodes, ReturnCheckBodyType::IfBody);
                    const ReturnCheckResult else_body_rcr = check_body(if_statement->else_body.nodes, ReturnCheckBodyType::ElseBody);
                    if (if_body_rcr.contains_return) {
                        if_statement->if_body.contains_return = true;
                    }
                    if (else_body_rcr.contains_return) {
                        if_statement->else_body.contains_return = true;
                    }
                    if (if_body_rcr.contains_return && else_body_rcr.contains_return) {
                        return handle_return(i, nodes, body_type);
                    }
                    break;
                }
                case ASTNodeType::ForStatement: {
                    ForStatement* for_statement = static_cast<ForStatement*>(nodes[i].get());
                    KPL_ASSERT_THAT(for_statement->body.contains_return == false,
                        "Flag whether a for body contains a 'return' statement must be false for return checking");
                    const ReturnCheckResult for_body_rcr = check_body(for_statement->body.nodes, ReturnCheckBodyType::ForBody);
                    if (for_body_rcr.contains_return) {
                        for_statement->body.contains_return = true;
                        return handle_return(i, nodes, body_type);
                    }
                    break;
                }
                case ASTNodeType::ReturnStatement:
                    return handle_return(i, nodes, body_type);
                default:
                    break;
            }
        }
        return {.contains_return = false, .contains_unreachable = false};
    }

    ReturnCheckResult ReturnCheckPass::handle_return(size_t index, const std::vector<std::unique_ptr<ASTNode>>& nodes, ReturnCheckBodyType body_type) {
        if (index < nodes.size() - 1) {
            const std::string message = std::format("Unreachable code in {} detected, starting from here:", body_type);
            diagnostic_sink.report(DiagnosticCode::UnreachableCode, std::move(message), nodes[index + 1]->source_location);
            return {.contains_return = true, .contains_unreachable = true};
        }
        return {.contains_return = true, .contains_unreachable = false};
    }

}
