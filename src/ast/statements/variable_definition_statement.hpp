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
#include "type_system/data_type_kind.hpp"
#include <memory>
#include <utility>

namespace kepler {

    struct VariableDefinitionStatement : Statement {
        DataTypeKind data_type;
        StringId identifier_id;
        std::unique_ptr<AssignmentStatement> assignment_statement;

        VariableDefinitionStatement(DataTypeKind data_type,
            StringId identifier_id,
            std::unique_ptr<AssignmentStatement> assignment_statement,
            SourceLocation source_location)
            : Statement(ASTNodeType::VariableDefinitionStatement, source_location),
              data_type(data_type),
              identifier_id(identifier_id),
              assignment_statement(std::move(assignment_statement)) {}
    };

}
