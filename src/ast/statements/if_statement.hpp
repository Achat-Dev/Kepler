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
#include "ast/expressions/expression.hpp"
#include "ast/statements/statement.hpp"
#include "diagnostics/source_location.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace kepler::ast {

    struct IfStatement : Statement {
        std::unique_ptr<Expression> condition;
        std::vector<std::unique_ptr<ASTNode>> if_body;
        std::vector<std::unique_ptr<ASTNode>> else_body;

        IfStatement(std::unique_ptr<Expression> condition, std::vector<std::unique_ptr<ASTNode>> if_body, std::vector<std::unique_ptr<ASTNode>> else_body, diagnostics::SourceLocation source_location)
            : Statement(ASTNodeType::IfStatement, source_location), condition(std::move(condition)), if_body(std::move(if_body)), else_body(std::move(else_body)) {}
    };

}
