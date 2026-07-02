// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/codegen_result.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/statement.hpp"
#include "semantic_analysis/string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <memory>

namespace kepler::ast {

    class VariableDefinitionStatement : public Statement {
    public:
        VariableDefinitionStatement(type_system::DataTypeKind data_type, semantic_analysis::StringId identifier_id, std::shared_ptr<AssignmentStatement> assignment_statement)
            : data_type(data_type), identifier_id(identifier_id), assignment_statement(assignment_statement) {}
        CodegenResult codegen() const override;

    private:
        const type_system::DataTypeKind data_type;
        const semantic_analysis::StringId identifier_id;
        const std::shared_ptr<AssignmentStatement> assignment_statement;
    };

}
