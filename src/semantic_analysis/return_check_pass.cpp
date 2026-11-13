// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/return_check_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    void ReturnCheckPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        for (const AbstractSyntaxTree& ast : asts) {
            for (ASTNode* node : ast.top_level_nodes) {
                KPL_ASSERT_THAT(node->node_kind != ASTNodeKind::Poison);
                switch (node->node_kind) {
                    case ASTNodeKind::Function: {
                        check_function(static_cast<Function*>(node));
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    void ReturnCheckPass::check_function(Function* function) {
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->body.contains_return == false);
        KPL_ASSERT_THAT(function->node_kind != ASTNodeKind::Poison);
        const ReturnCheckResult return_result = check_body(function->body.nodes, ReturnCheckBodyKind::FunctionBody);
        if (return_result.contains_return) {
            function->body.contains_return = true;
            return;
        }

        const Type* void_type = type_table.lookup(get_builtin_type_id(TypeKind::Void));
        if (function->prototype->return_type_identifier_id == void_type->identifier_id) {
            return;
        }

        const StringId identifier_id = get_prototype_identifier_id(function->prototype);
        const std::string message = std::format("Not all code paths of function '{}' contain a return statement",
            StringPool::get().lookup(identifier_id));
        diagnostic_sink.report(DiagnosticCode::MissingReturn, std::move(message), function->source_location);
    }

    ReturnCheckResult ReturnCheckPass::check_body(const std::vector<ASTNode*>& nodes, ReturnCheckBodyKind body_kind) {
        for (size_t i = 0; i < nodes.size(); i++) {
            KPL_ASSERT_THAT(nodes[i]->node_kind != ASTNodeKind::Poison);
            switch (nodes[i]->node_kind) {
                case ASTNodeKind::IfStatement: {
                    IfStatement* if_statement = static_cast<IfStatement*>(nodes[i]);
                    KPL_ASSERT_THAT(if_statement->if_body.contains_return == false);
                    KPL_ASSERT_THAT(if_statement->else_body.contains_return == false);
                    const ReturnCheckResult if_body_rcr = check_body(if_statement->if_body.nodes, ReturnCheckBodyKind::IfBody);
                    const ReturnCheckResult else_body_rcr = check_body(if_statement->else_body.nodes, ReturnCheckBodyKind::ElseBody);
                    if (if_body_rcr.contains_return) {
                        if_statement->if_body.contains_return = true;
                    }
                    if (else_body_rcr.contains_return) {
                        if_statement->else_body.contains_return = true;
                    }
                    if (if_body_rcr.contains_return && else_body_rcr.contains_return) {
                        return handle_return(i, nodes, body_kind);
                    }
                    break;
                }
                case ASTNodeKind::ForStatement: {
                    ForStatement* for_statement = static_cast<ForStatement*>(nodes[i]);
                    KPL_ASSERT_THAT(for_statement->body.contains_return == false);
                    const ReturnCheckResult for_body_rcr = check_body(for_statement->body.nodes, ReturnCheckBodyKind::ForBody);
                    if (for_body_rcr.contains_return) {
                        for_statement->body.contains_return = true;
                        return handle_return(i, nodes, body_kind);
                    }
                    break;
                }
                case ASTNodeKind::ReturnStatement:
                    return handle_return(i, nodes, body_kind);
                default:
                    break;
            }
        }
        return {.contains_return = false, .contains_unreachable = false};
    }

    ReturnCheckResult ReturnCheckPass::handle_return(size_t index, const std::vector<ASTNode*>& nodes, ReturnCheckBodyKind body_kind) {
        if (index < nodes.size() - 1) {
            const std::string message = std::format("Unreachable code in {} detected, starting from here:", body_kind);
            diagnostic_sink.report(DiagnosticCode::UnreachableCode, std::move(message), nodes[index + 1]->source_location);
            return {.contains_return = true, .contains_unreachable = true};
        }
        return {.contains_return = true, .contains_unreachable = false};
    }

}
