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
#include "diagnostics/source_location.hpp"
#include "utils/string_pool.hpp"
#include <utility>
#include <vector>

namespace kepler {

    struct StructMember {
        StringId type_identifier_id;
        StringId identifier_id;
        SourceLocation type_source_location;
        SourceLocation identifier_source_location;
    };

    struct Struct : ExportableNode {
        StringId identifier_id;
        std::vector<StructMember> members;

        Struct(StringId identifier_id, std::vector<StructMember> members, LinkageType linkage_type, SourceLocation source_location)
            : ExportableNode(ASTNodeType::Struct, linkage_type, std::move(source_location)), identifier_id(identifier_id), members(std::move(members)) {}
    };

}
