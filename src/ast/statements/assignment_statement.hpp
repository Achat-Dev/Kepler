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
#include "ast/expressions/variable_expression.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct AssignmentStatement : Statement {
        std::unique_ptr<VariableExpression> variable_expression;
        std::unique_ptr<Expression> value_expression;

        AssignmentStatement(std::unique_ptr<VariableExpression> variable_expression,
            std::unique_ptr<Expression> value_expression,
            SourceLocation source_location)
            : Statement(ASTNodeType::AssignmentStatement, std::move(source_location)),
              variable_expression(std::move(variable_expression)),
              value_expression(std::move(value_expression)) {}
    };

}
