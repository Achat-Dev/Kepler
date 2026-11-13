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
#include "ast/function.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include <cstddef>
#include <format>
#include <string>
#include <vector>

namespace kepler {

    enum class ReturnCheckBodyKind {
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
        ReturnCheckPass(DiagnosticSink& diagnostic_sink, TypeTable& type_table)
            : diagnostic_sink(diagnostic_sink), type_table(type_table) {}
        void run(std::vector<AbstractSyntaxTree>& asts) override;

    private:
        DiagnosticSink& diagnostic_sink;
        TypeTable& type_table;

        void check_function(Function* function);
        ReturnCheckResult check_body(const std::vector<ASTNode*>& nodes, ReturnCheckBodyKind body_kind);
        ReturnCheckResult handle_return(size_t index, const std::vector<ASTNode*>& nodes, ReturnCheckBodyKind body_kind);
    };
}

template <>
struct std::formatter<kepler::ReturnCheckBodyKind> : std::formatter<std::string> {
    auto format(const kepler::ReturnCheckBodyKind& body_kind, std::format_context& ctx) const {
        switch (body_kind) {
            case kepler::ReturnCheckBodyKind::FunctionBody:
                return std::formatter<std::string>::format("function body", ctx);
            case kepler::ReturnCheckBodyKind::ForBody:
                return std::formatter<std::string>::format("'for' body", ctx);
            case kepler::ReturnCheckBodyKind::IfBody:
                return std::formatter<std::string>::format("'if' body", ctx);
            case kepler::ReturnCheckBodyKind::ElseBody:
                return std::formatter<std::string>::format("'else' body", ctx);
        }
        KPL_ASSERT_UNREACHABLE("Missing format implementation for return check body kind '{}'", static_cast<int>(body_kind));
    }
};
