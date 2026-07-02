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
#include "type_system/data_type_kind.hpp"
#include <memory>

namespace kepler::ast {

    class CastExpression : public Expression {
    public:
        CastExpression(std::shared_ptr<Expression> expression, type_system::DataTypeKind target_data_type)
            : expression(expression), target_data_type(target_data_type) {}
        CodegenResult codegen() const override;

    private:
        const std::shared_ptr<Expression> expression;
        const type_system::DataTypeKind target_data_type;
    };

}
