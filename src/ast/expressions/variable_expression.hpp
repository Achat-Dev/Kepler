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
#include <utility>

namespace kepler {

    struct VariableExpression : Expression {
        StringId identifier_id;
        Symbol* symbol;

        VariableExpression(StringId identifier_id, SourceLocation source_location)
            : Expression(ASTNodeType::VariableExpression, source_location), identifier_id(std::move(identifier_id)) {}
    };

}
