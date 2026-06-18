// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/expression.hpp"

#include <memory>
#include <vector>

namespace Kepler::AST {

    class IfExpression : public Expression {
    private:
        std::unique_ptr<Expression> condition;
        std::vector<std::unique_ptr<Expression>> if_body;
        std::vector<std::unique_ptr<Expression>> else_body;

    public:
        IfExpression(std::unique_ptr<Expression> condition,
            std::vector<std::unique_ptr<Expression>> if_body,
            std::vector<std::unique_ptr<Expression>> else_body)
            : condition(std::move(condition)),
              if_body(std::move(if_body)),
              else_body(std::move(else_body)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
