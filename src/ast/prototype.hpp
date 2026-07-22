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
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    struct ParameterData {
        DataTypeKind data_type;
        StringId identifier_id;
        SourceLocation source_location;
    };

    struct Prototype : ASTNode {
        enum class LinkageType {
            Internal,
            External,
        };

        LinkageType linkage_type;
        DataTypeKind return_type;
        StringId identifier_id;
        std::vector<ParameterData> parameter_data;

        Prototype(LinkageType linkage_type,
            DataTypeKind return_type,
            StringId identifier_id,
            std::vector<ParameterData> parameter_data,
            SourceLocation source_location)
            : ASTNode(ASTNodeType::Prototype, std::move(source_location)),
              linkage_type(linkage_type),
              return_type(return_type),
              identifier_id(identifier_id),
              parameter_data(std::move(parameter_data)) {}
    };

}

template <>
struct std::formatter<kepler::Prototype::LinkageType> : std::formatter<std::string> {
    auto format(const kepler::Prototype::LinkageType& linkage_type, std::format_context& ctx) const {
        switch (linkage_type) {
            case kepler::Prototype::LinkageType::Internal:
                return std::formatter<std::string>::format("Internal", ctx);
            case kepler::Prototype::LinkageType::External:
                return std::formatter<std::string>::format("External", ctx);
            default:
                kepler::log::warning("Missing format implementation for linkage type '{}'", static_cast<int>(linkage_type));
                return std::formatter<std::string>::format(std::format("{}", static_cast<int>(linkage_type)), ctx);
        }
    }
};
