// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "utils/assert.h"
#include <format>
#include <string>
#include <utility>

namespace kepler {

    enum class ASTNodeType {
        Poison,
        Extern,
        Function,
        Prototype,
        AssignmentStatement,
        ForStatement,
        IfStatement,
        ReturnStatement,
        VariableDefinitionStatement,
        BooleanLiteralExpression,
        FloatingPointLiteralExpression,
        IntegerLiteralExpression,
        StringLiteralExpression,
        BinaryExpression,
        CallExpression,
        CastExpression,
        MathematicalNegationExpression,
        VariableExpression,
    };

    struct ASTNode {
        ASTNodeType node_type;
        SourceLocation source_location;

        ASTNode(ASTNodeType node_type, SourceLocation source_location)
            : node_type(node_type), source_location(std::move(source_location)) {}
        virtual ~ASTNode() = default;
    };

}

template <>
struct std::formatter<kepler::ASTNodeType> : std::formatter<std::string> {
    auto format(const kepler::ASTNodeType& ast_node_type, std::format_context& ctx) const {
        switch (ast_node_type) {
            case kepler::ASTNodeType::Poison:
                return std::formatter<std::string>::format("Poison", ctx);
            case kepler::ASTNodeType::Extern:
                return std::formatter<std::string>::format("Extern", ctx);
            case kepler::ASTNodeType::Function:
                return std::formatter<std::string>::format("Function", ctx);
            case kepler::ASTNodeType::Prototype:
                return std::formatter<std::string>::format("Prototype", ctx);
            case kepler::ASTNodeType::AssignmentStatement:
                return std::formatter<std::string>::format("AssignmentStatement", ctx);
            case kepler::ASTNodeType::ForStatement:
                return std::formatter<std::string>::format("ForStatement", ctx);
            case kepler::ASTNodeType::IfStatement:
                return std::formatter<std::string>::format("IfStatement", ctx);
            case kepler::ASTNodeType::ReturnStatement:
                return std::formatter<std::string>::format("ReturnStatement", ctx);
            case kepler::ASTNodeType::VariableDefinitionStatement:
                return std::formatter<std::string>::format("VariableDefinitionStatement", ctx);
            case kepler::ASTNodeType::BooleanLiteralExpression:
                return std::formatter<std::string>::format("BooleanLiteralExpression", ctx);
            case kepler::ASTNodeType::FloatingPointLiteralExpression:
                return std::formatter<std::string>::format("FloatingPointLiteralExpression", ctx);
            case kepler::ASTNodeType::IntegerLiteralExpression:
                return std::formatter<std::string>::format("IntegerLiteralExpression", ctx);
            case kepler::ASTNodeType::StringLiteralExpression:
                return std::formatter<std::string>::format("StringLiteralExpression", ctx);
            case kepler::ASTNodeType::BinaryExpression:
                return std::formatter<std::string>::format("BinaryExpression", ctx);
            case kepler::ASTNodeType::CallExpression:
                return std::formatter<std::string>::format("CallExpression", ctx);
            case kepler::ASTNodeType::CastExpression:
                return std::formatter<std::string>::format("CastExpression", ctx);
            case kepler::ASTNodeType::MathematicalNegationExpression:
                return std::formatter<std::string>::format("MathematicalNegationExpression", ctx);
            case kepler::ASTNodeType::VariableExpression:
                return std::formatter<std::string>::format("VariableExpression", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for ast node type '{}'", static_cast<int>(ast_node_type));
    }
};
