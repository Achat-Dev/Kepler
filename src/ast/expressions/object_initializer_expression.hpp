// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
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
#include <vector>

namespace kepler {

    struct MemberInitializerData {
        StringId identifier_id;
        SourceLocation source_location;
        std::unique_ptr<Expression> value_expression;
    };

    struct ObjectInitializerExpression : Expression {
        StringId type_identifier_id;
        TypeId type_id;
        std::vector<MemberInitializerData> member_initializers;
        SourceLocation type_source_location;

        ObjectInitializerExpression(StringId type_identifier_id,
            std::vector<MemberInitializerData> member_initializers,
            SourceLocation source_location,
            SourceLocation type_source_location)
            : Expression(ASTNodeType::ObjectInitializerExpression, std::move(source_location)),
              type_identifier_id(type_identifier_id),
              member_initializers(std::move(member_initializers)),
              type_source_location(std::move(type_source_location)) {}
    };

}
