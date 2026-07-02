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
#include "semantic_analysis/symbol_id.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace kepler::ast {

    class Function : public ASTNode {
    public:
        Function(semantic_analysis::SymbolId prototype_id, std::vector<std::shared_ptr<ASTNode>> body)
            : prototype_id(prototype_id), body(std::move(body)) {}
        CodegenResult codegen() const;

    private:
        const semantic_analysis::SymbolId prototype_id;
        const std::vector<std::shared_ptr<ASTNode>> body;
    };

}
