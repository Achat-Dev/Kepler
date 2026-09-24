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
#include <memory>
#include <vector>

namespace kepler {

    struct AbstractSyntaxTree {
        std::unique_ptr<ModuleStatement> module_statement;
        std::vector<std::unique_ptr<ImportStatement>> import_statements;
        std::vector<std::unique_ptr<Struct>> struct_nodes;
        std::vector<std::unique_ptr<ASTNode>> top_level_nodes;
    };

}
