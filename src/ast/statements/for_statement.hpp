// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/statements/statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/source_location.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace kepler::ast {

    struct ForStatement : Statement {
        std::unique_ptr<VariableDefinitionStatement> loop_variable_definition;
        std::unique_ptr<Expression> end_value;
        std::unique_ptr<Expression> step_value;
        std::vector<std::unique_ptr<ASTNode>> body;

        ForStatement(std::unique_ptr<VariableDefinitionStatement> loop_variable_definition,
            std::unique_ptr<Expression> end_value,
            std::unique_ptr<Expression> step_value,
            std::vector<std::unique_ptr<ASTNode>> body,
            diagnostics::SourceLocation source_location)
            : Statement(ASTNodeType::ForStatement, std::move(source_location)),
              loop_variable_definition(std::move(loop_variable_definition)),
              end_value(std::move(end_value)),
              step_value(std::move(step_value)),
              body(std::move(body)) {}
    };

}
