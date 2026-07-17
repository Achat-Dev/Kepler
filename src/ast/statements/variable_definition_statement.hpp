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
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include "type_system/data_type_kind.hpp"
#include <memory>
#include <string>
#include <utility>

namespace kepler::ast {

    struct VariableDefinitionStatement : Statement {
        type_system::DataTypeKind data_type;
        std::string identifier;
        std::unique_ptr<AssignmentStatement> assignment_statement;

        VariableDefinitionStatement(type_system::DataTypeKind data_type, std::string identifier, std::unique_ptr<AssignmentStatement> assignment_statement, diagnostics::SourceLocation source_location)
            : Statement(ASTNodeType::VariableDefinitionStatement, source_location), data_type(data_type), identifier(std::move(identifier)), assignment_statement(std::move(assignment_statement)) {}
    };

}
