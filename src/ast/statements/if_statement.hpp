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
#include <memory>
#include <utility>
#include <vector>

namespace kepler::ast {

    class IfStatement : public Statement {
    public:
        IfStatement(std::shared_ptr<Expression> condition, std::vector<std::shared_ptr<ASTNode>> if_body, std::vector<std::shared_ptr<ASTNode>> else_body)
            : condition(condition), if_body(std::move(if_body)), else_body(std::move(else_body)) {}
        CodegenResult codegen() const override;

    private:
        const std::shared_ptr<Expression> condition;
        const std::vector<std::shared_ptr<ASTNode>> if_body;
        const std::vector<std::shared_ptr<ASTNode>> else_body;
    };

}
