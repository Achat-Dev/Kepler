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
#include "ast/codegen_result.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/statements/statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include <memory>
#include <utility>

namespace kepler::ast {

    class ForStatement : public Statement {
    public:
        ForStatement(std::shared_ptr<VariableDefinitionStatement> loop_variable_definition,
            std::shared_ptr<Expression> end_value,
            std::shared_ptr<Expression> step_value,
            std::vector<std::shared_ptr<ASTNode>> body)
            : loop_variable_definition(loop_variable_definition),
              end_value(end_value),
              step_value(step_value),
              body(std::move(body)) {}
        CodegenResult codegen() const override;

    private:
        const std::shared_ptr<VariableDefinitionStatement> loop_variable_definition;
        const std::shared_ptr<Expression> end_value;
        const std::shared_ptr<Expression> step_value;
        const std::vector<std::shared_ptr<ASTNode>> body;
    };

}
