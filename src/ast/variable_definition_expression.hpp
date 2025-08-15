// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/binary_expression.hpp"
#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "types/type_token.hpp"

#include <memory>
#include <string>

namespace Kepler::AST {

    class VariableDefinitionExpression: public Expression {
    private:
        const Type::TypeToken type;
        const std::string name;
        std::unique_ptr<BinaryExpression> value;

    public:
        VariableDefinitionExpression(Type::TypeToken type, const std::string& name, std::unique_ptr<BinaryExpression> value)
            : type(type), name(name), value(std::move(value)) {}
        std::unique_ptr<ExpressionResult> codegen() override;
        const std::string& get_name() const;
    };

}
