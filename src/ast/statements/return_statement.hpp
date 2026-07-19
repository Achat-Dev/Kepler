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
#include <memory>
#include <utility>

namespace kepler {

    struct ReturnStatement : Statement {
        std::unique_ptr<Expression> expression;

        ReturnStatement(std::unique_ptr<Expression> expression, SourceLocation source_location)
            : Statement(ASTNodeType::ReturnStatement, source_location), expression(std::move(expression)) {}
    };

}
