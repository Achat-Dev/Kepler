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
#include "semantic_analysis/symbol.hpp"
#include "utils/string_pool.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace kepler {

    struct CallExpression : Expression {
        StringId identifier_id;
        Symbol* symbol;
        std::vector<std::unique_ptr<Expression>> args;

        CallExpression(StringId identifier_id, std::vector<std::unique_ptr<Expression>> args, SourceLocation source_location)
            : Expression(ASTNodeType::CallExpression, std::move(source_location)), identifier_id(identifier_id), args(std::move(args)) {}
    };

}
