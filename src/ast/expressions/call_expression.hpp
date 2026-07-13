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
#include "ast/expressions/expression.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kepler::ast {

    class CallExpression : public Expression {
    public:
        CallExpression(std::string identifier, std::vector<std::shared_ptr<Expression>> args)
            : identifier(std::move(identifier)), args(std::move(args)) {}
        CodegenResult codegen() const override;

    private:
        const std::string identifier;
        const std::vector<std::shared_ptr<Expression>> args;
    };

}
