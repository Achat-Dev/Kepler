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
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include "utils/identifier_path.hpp"
#include <utility>
#include <vector>

namespace kepler {

    struct CastData {
        bool is_cast = false;
        bool is_builtin_cast = false;
        TypeId original_argument_type_id;
    };

    struct CallExpression : Expression {
        IdentifierPath identifier_path;
        SymbolId symbol_id;
        CastData cast_data;
        IdentifierPath module_path;
        std::vector<Expression*> arguments;
        std::vector<SymbolId> candidate_symbol_ids;

        CallExpression(IdentifierPath identifier_path,
            std::vector<Expression*> arguments,
            IdentifierPath module_path,
            SourceLocation source_location)
            : Expression(ASTNodeType::CallExpression, std::move(source_location)),
              identifier_path(std::move(identifier_path)),
              module_path(std::move(module_path)),
              arguments(std::move(arguments)) {}
    };

}
