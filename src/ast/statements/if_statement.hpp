// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include <utility>
#include <vector>

namespace kepler {

    struct IfStatement : Statement {
        Expression* condition;
        NodeBody if_body;
        NodeBody else_body;

        IfStatement(Expression* condition,
            std::vector<ASTNode*> if_body,
            std::vector<ASTNode*> else_body,
            SourceLocation source_location)
            : Statement(ASTNodeKind::IfStatement, std::move(source_location)),
              condition(condition),
              if_body{.nodes = std::move(if_body)},
              else_body{.nodes = std::move(else_body)} {}
    };

}
