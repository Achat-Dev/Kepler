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
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/source_location.hpp"
#include <utility>
#include <vector>

namespace kepler {

    struct ForStatement : Statement {
        VariableDefinitionStatement* loop_variable_definition;
        Expression* end_value;
        Expression* step_value;
        NodeBody body;

        ForStatement(VariableDefinitionStatement* loop_variable_definition,
            Expression* end_value,
            Expression* step_value,
            std::vector<ASTNode*> body,
            SourceLocation source_location)
            : Statement(ASTNodeKind::ForStatement, std::move(source_location)),
              loop_variable_definition(loop_variable_definition),
              end_value(end_value),
              step_value(step_value),
              body{.nodes = std::move(body)} {}
    };

}
