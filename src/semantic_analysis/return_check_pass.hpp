// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_pass.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <vector>

namespace kepler {

    enum class ReturnCheckBodyType {
        FunctionBody,
        ForBody,
        IfBody,
        ElseBody,
    };

    struct ReturnCheckResult {
        bool contains_return = false;
        bool contains_unreachable = false;
    };

    class ReturnCheckPass : public ASTPass<void> {
    public:
        ReturnCheckPass(AbstractSyntaxTree& ast, DiagnosticSink& diagnostic_sink, const TypeTable& type_table)
            : ASTPass(ast), diagnostic_sink(diagnostic_sink), type_table(type_table) {}
        void run() override;

    private:
        DiagnosticSink& diagnostic_sink;
        const TypeTable& type_table;

        ReturnCheckResult check_body(const std::vector<std::unique_ptr<ASTNode>>& nodes, ReturnCheckBodyType body_type);
        ReturnCheckResult handle_return(size_t index, const std::vector<std::unique_ptr<ASTNode>>& nodes, ReturnCheckBodyType body_type);
    };
}

template <>
struct std::formatter<kepler::ReturnCheckBodyType> : std::formatter<std::string> {
    auto format(const kepler::ReturnCheckBodyType& body_type, std::format_context& ctx) const {
        switch (body_type) {
            case kepler::ReturnCheckBodyType::FunctionBody:
                return std::formatter<std::string>::format("function body", ctx);
            case kepler::ReturnCheckBodyType::ForBody:
                return std::formatter<std::string>::format("'for' body", ctx);
            case kepler::ReturnCheckBodyType::IfBody:
                return std::formatter<std::string>::format("'if' body", ctx);
            case kepler::ReturnCheckBodyType::ElseBody:
                return std::formatter<std::string>::format("'else' body", ctx);
        }
        KPL_ASSERT_UNREACHABLE("Missing format implementation for return check body type '{}'", static_cast<int>(body_type));
    }
};
