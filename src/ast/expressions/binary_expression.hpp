// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/expressions/expression.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct BinaryExpression : Expression {
        OperatorType operator_type;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

        BinaryExpression(OperatorType operator_type,
            std::unique_ptr<Expression> lhs,
            std::unique_ptr<Expression> rhs,
            SourceLocation source_location)
            : Expression(ASTNodeType::BinaryExpression, std::move(source_location)),
              operator_type(operator_type),
              lhs(std::move(lhs)),
              rhs(std::move(rhs)) {}
    };

}
