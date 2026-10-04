// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include <memory>
#include <utility>
#include <variant>

namespace kepler {

    using AssignmentTargetExpression = std::variant<std::unique_ptr<MemberExpression>, std::unique_ptr<VariableExpression>>;

    struct AssignmentStatement : Statement {
        AssignmentTargetExpression assignment_target_expression;
        std::unique_ptr<Expression> value_expression;

        AssignmentStatement(AssignmentTargetExpression assignment_target_expression,
            std::unique_ptr<Expression> value_expression,
            SourceLocation source_location)
            : Statement(ASTNodeType::AssignmentStatement, std::move(source_location)),
              assignment_target_expression(std::move(assignment_target_expression)),
              value_expression(std::move(value_expression)) {}
    };

}
