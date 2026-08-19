// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "diagnostics/source_location.hpp"
#include "type_system/type.hpp"
#include "utils/string_pool.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct CastExpression : Expression {
        StringId target_type_id;
        Type* original_type = nullptr;
        Type* target_type = nullptr;
        std::unique_ptr<Expression> expression;

        CastExpression(StringId target_type_id, std::unique_ptr<Expression> expression, SourceLocation source_location)
            : Expression(ASTNodeType::CastExpression, std::move(source_location)), target_type_id(target_type_id), expression(std::move(expression)) {}
    };

}
