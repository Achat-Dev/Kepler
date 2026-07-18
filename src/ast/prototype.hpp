// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "diagnostics/source_location.hpp"
#include "type_system/data_type_kind.hpp"
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    struct ParameterData {
        DataTypeKind data_type;
        std::string identifier;
    };

    struct Prototype : ASTNode {
        enum class LinkageType {
            Internal,
            External,
        };

        LinkageType linkage_type;
        DataTypeKind return_type;
        std::string identifier;
        std::vector<ParameterData> parameter_data;

        Prototype(LinkageType linkage_type, DataTypeKind return_type, std::string identifier, std::vector<ParameterData> parameter_data, SourceLocation source_location)
            : ASTNode(ASTNodeType::Prototype, std::move(source_location)), return_type(return_type), identifier(std::move(identifier)), parameter_data(std::move(parameter_data)) {}
    };

}
