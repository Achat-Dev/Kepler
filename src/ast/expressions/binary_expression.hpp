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
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include <utility>

namespace kepler {

    struct BinaryExpression : Expression {
        OperatorType operator_type;
        Expression* lhs;
        Expression* rhs;
        TypeId target_type_id;
        SymbolId method_symbol_id;

        BinaryExpression(OperatorType operator_type,
            Expression* lhs,
            Expression* rhs,
            SourceLocation source_location)
            : Expression(ASTNodeType::BinaryExpression, std::move(source_location)),
              operator_type(operator_type),
              lhs(lhs),
              rhs(rhs) {}
    };

}
