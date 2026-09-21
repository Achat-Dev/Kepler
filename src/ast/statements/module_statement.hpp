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
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include <utility>

namespace kepler {

    struct ModuleStatement : Statement {
        ModuleId module_id;
        ModulePath module_path;

        ModuleStatement(ModulePath module_path, SourceLocation source_location)
            : Statement(ASTNodeType::ModuleStatement, std::move(source_location)),
              module_path(std::move(module_path)) {}
    };

}
