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
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "utils/string_pool.hpp"
#include <utility>
#include <vector>

namespace kepler {

    struct ParameterData {
        StringId type_identifier_id;
        StringId identifier_id;
        TypeId type_id;
        SymbolId symbol_id;
        SourceLocation type_source_location;
        SourceLocation identifier_source_location;
    };

    struct Prototype : ASTNode {
        SourceLocation identifier_source_location;
        StringId identifier_id;
        StringId return_type_identifier_id;
        TypeId return_type_id;
        SymbolId symbol_id;
        std::vector<ParameterData> parameter_data;
        bool is_variadic;

        Prototype(StringId return_type_identifier_id,
            StringId identifier_id,
            std::vector<ParameterData> parameter_data,
            bool is_variadic,
            SourceLocation type_source_location,
            SourceLocation identifier_source_location)
            : ASTNode(ASTNodeType::Prototype, std::move(type_source_location)),
              identifier_source_location(std::move(identifier_source_location)),
              return_type_identifier_id(return_type_identifier_id),
              identifier_id(identifier_id),
              parameter_data(std::move(parameter_data)),
              is_variadic(is_variadic) {}
    };

}
