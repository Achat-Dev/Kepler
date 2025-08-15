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
#include "ast/expression_result.hpp"

#include <memory>

namespace Kepler::AST {

    class BooleanLiteralExpression: public Expression {
    private:
        bool value;

    public:
        BooleanLiteralExpression(bool value): value(value) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
