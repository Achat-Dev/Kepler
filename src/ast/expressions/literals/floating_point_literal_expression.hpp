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
#include "type_system/type.hpp"
#include <utility>

namespace kepler {

    struct FloatingPointLiteralExpression : Expression {
        double value;
        Type* target_type;

        FloatingPointLiteralExpression(double value, SourceLocation source_location)
            : Expression(ASTNodeType::FloatingPointLiteralExpression, std::move(source_location)),
              value(value),
              target_type(nullptr) {}
    };

}
