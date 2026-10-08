// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/expressions/expression.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/identifier_path.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace kepler {

    struct MemberAccessData {
        TypeId struct_type_id;
        uint32_t member_index;
    };

    struct MemberFunctionIdentifier {
        StringId type_identifier_id;
        StringId identifier_id;
        SourceLocation type_source_location;
        SourceLocation identifier_source_location;
    };

    struct MemberExpression : Expression {
        IdentifierPath member_path;
        SymbolId object_symbol_id;
        TypeId member_type_id;
        std::vector<MemberAccessData> access_data;

        MemberExpression(IdentifierPath member_path)
            : Expression(ASTNodeType::MemberExpression, member_path.source_location), member_path(std::move(member_path)) {}
    };

}
