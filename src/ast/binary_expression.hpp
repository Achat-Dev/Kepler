// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "expression.hpp"
#include "expression_result.hpp"
#include "lexer.hpp"

#include <memory>

namespace Kepler::AST {

    class BinaryExpression : public Expression {
    private:
        Lexer::Token op;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

    public:
        BinaryExpression(Lexer::Token op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        Lexer::Token get_operator() const;
    };

}
