// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/expressions/expression.hpp"
#include "diagnostics/source_location.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct MathematicalNegationExpression : Expression {
        std::unique_ptr<Expression> expression;

        MathematicalNegationExpression(std::unique_ptr<Expression> expression, SourceLocation source_location)
            : Expression(ASTNodeType::MathematicalNegationExpression, std::move(source_location)), expression(std::move(expression)) {}
    };

}
