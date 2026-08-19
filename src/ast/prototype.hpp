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
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <llvm/IR/Function.h>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    struct Symbol;

    struct ParameterData {
        StringId type_id;
        StringId identifier_id;
        Type* type = nullptr;
        Symbol* symbol = nullptr;
        SourceLocation type_source_location;
        SourceLocation identifier_source_location;
    };

    struct Prototype : ASTNode {
        enum class LinkageType {
            Internal,
            External,
        };

        LinkageType linkage_type;
        SourceLocation identifier_source_location;
        StringId identifier_id;
        StringId return_type_id;
        Type* return_type = nullptr;
        Symbol* symbol = nullptr;
        std::vector<ParameterData> parameter_data;

        Prototype(LinkageType linkage_type,
            StringId return_type_id,
            StringId identifier_id,
            std::vector<ParameterData> parameter_data,
            SourceLocation type_source_location,
            SourceLocation identifier_source_location)
            : ASTNode(ASTNodeType::Prototype, std::move(type_source_location)),
              linkage_type(linkage_type),
              identifier_source_location(std::move(identifier_source_location)),
              return_type_id(return_type_id),
              identifier_id(identifier_id),
              parameter_data(std::move(parameter_data)) {}
    };

    llvm::Function::LinkageTypes get_llvm_linkage_type(Prototype::LinkageType linkage_type);

}

template <>
struct std::formatter<kepler::Prototype::LinkageType> : std::formatter<std::string> {
    auto format(const kepler::Prototype::LinkageType& linkage_type, std::format_context& ctx) const {
        switch (linkage_type) {
            case kepler::Prototype::LinkageType::Internal:
                return std::formatter<std::string>::format("Internal", ctx);
            case kepler::Prototype::LinkageType::External:
                return std::formatter<std::string>::format("External", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for linkage type '{}'", static_cast<int>(linkage_type));
    }
};
