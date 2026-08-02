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
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include "string_pool.hpp"
#include "type_system/type.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct VariableDefinitionStatement : Statement {
        StringId type_id;
        StringId identifier_id;
        Type* type;
        std::unique_ptr<AssignmentStatement> assignment_statement;

        VariableDefinitionStatement(StringId type_id,
            StringId identifier_id,
            std::unique_ptr<AssignmentStatement> assignment_statement,
            SourceLocation source_location)
            : Statement(ASTNodeType::VariableDefinitionStatement, source_location),
              type_id(type_id),
              type(nullptr),
              identifier_id(identifier_id),
              assignment_statement(std::move(assignment_statement)) {}
    };

}
