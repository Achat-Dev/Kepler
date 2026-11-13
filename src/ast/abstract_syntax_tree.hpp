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
#include "ast/statements/import_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "ast/struct.hpp"
#include "utils/arena_allocator.hpp"
#include <vector>

namespace kepler {

    struct AbstractSyntaxTree {
        ArenaAllocator allocator;
        ModuleStatement* module_statement;
        std::vector<ImportStatement*> import_statements;
        std::vector<Struct*> struct_nodes;
        std::vector<ASTNode*> top_level_nodes;
    };

}
