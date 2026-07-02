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
#include "semantic_analysis/string_table.hpp"

namespace kepler::ast {

    class VariableExpression : public Expression {
    public:
        VariableExpression(semantic_analysis::StringId identifier_id)
            : identifier_id(identifier_id) {}
        CodegenResult codegen() const override;

    private:
        const semantic_analysis::StringId identifier_id;
    };

}
