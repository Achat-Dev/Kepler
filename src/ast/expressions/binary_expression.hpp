// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/codegen_result.hpp"
#include "ast/expressions/expression.hpp"
#include "lexer/operator_type.hpp"
#include <memory>

namespace kepler::ast {

    class BinaryExpression : public Expression {
    public:
        BinaryExpression(lexer::OperatorType operator_type, std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
            : operator_type(operator_type), lhs(lhs), rhs(rhs) {}
        CodegenResult codegen() const override;

    private:
        const lexer::OperatorType operator_type;
        const std::shared_ptr<Expression> lhs;
        const std::shared_ptr<Expression> rhs;
    };

}
