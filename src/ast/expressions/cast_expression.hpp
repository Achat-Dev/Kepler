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
#include "type_system/data_type_kind.hpp"
#include <memory>
#include <utility>

namespace kepler::ast {

    struct CastExpression : Expression {
        type_system::DataTypeKind target_data_type;
        std::unique_ptr<Expression> expression;

        CastExpression(type_system::DataTypeKind target_data_type, std::unique_ptr<Expression> expression, diagnostics::SourceLocation source_location)
            : Expression(ASTNodeType::CastExpression, std::move(source_location)), target_data_type(target_data_type), expression(std::move(expression)) {}
    };

}
