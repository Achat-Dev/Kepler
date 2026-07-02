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

namespace kepler::ast {

    class Prototype : public ASTNode {
    public:
        enum class LinkageType {
            Internal,
            External,
        };

        Prototype(semantic_analysis::SymbolId prototype_id) : prototype_id(prototype_id) {}
        CodegenResult codegen() const;

    private:
        const semantic_analysis::SymbolId prototype_id;
    };

}
