// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "log.hpp"
#include <format>
#include <string>
#include <utility>

namespace kepler::ast {

    enum class ASTNodeType {
        Extern,
        Function,
        Prototype,
        Statement,
        AssignmentStatement,
        ForStatement,
        IfStatement,
        ReturnStatement,
        VariableDefinitionStatement,
        Expression,
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
        diagnostics::SourceLocation source_location;

        ASTNode(ASTNodeType node_type, diagnostics::SourceLocation source_location)
            : node_type(node_type), source_location(std::move(source_location)) {}
        virtual ~ASTNode() = default;
    };

}

template <>
struct std::formatter<kepler::ast::ASTNodeType> : std::formatter<std::string> {
    auto format(const kepler::ast::ASTNodeType& ast_node_type, std::format_context& ctx) const {
        switch (ast_node_type) {
            case kepler::ast::ASTNodeType::Function:
                return std::formatter<std::string>::format("Function", ctx);
            case kepler::ast::ASTNodeType::Prototype:
                return std::formatter<std::string>::format("Prototype", ctx);
            case kepler::ast::ASTNodeType::Statement:
                return std::formatter<std::string>::format("Statement", ctx);
            case kepler::ast::ASTNodeType::AssignmentStatement:
                return std::formatter<std::string>::format("AssignmentStatement", ctx);
            case kepler::ast::ASTNodeType::ForStatement:
                return std::formatter<std::string>::format("ForStatement", ctx);
            case kepler::ast::ASTNodeType::IfStatement:
                return std::formatter<std::string>::format("IfStatement", ctx);
            case kepler::ast::ASTNodeType::ReturnStatement:
                return std::formatter<std::string>::format("ReturnStatement", ctx);
            case kepler::ast::ASTNodeType::VariableDefinitionStatement:
                return std::formatter<std::string>::format("VariableDefinitionStatement", ctx);
            case kepler::ast::ASTNodeType::Expression:
                return std::formatter<std::string>::format("Expression", ctx);
            case kepler::ast::ASTNodeType::BooleanLiteralExpression:
                return std::formatter<std::string>::format("BooleanLiteralExpression", ctx);
            case kepler::ast::ASTNodeType::FloatingPointLiteralExpression:
                return std::formatter<std::string>::format("FloatingPointLiteralExpression", ctx);
            case kepler::ast::ASTNodeType::IntegerLiteralExpression:
                return std::formatter<std::string>::format("IntegerLiteralExpression", ctx);
            case kepler::ast::ASTNodeType::StringLiteralExpression:
                return std::formatter<std::string>::format("StringLiteralExpression", ctx);
            case kepler::ast::ASTNodeType::BinaryExpression:
                return std::formatter<std::string>::format("BinaryExpression", ctx);
            case kepler::ast::ASTNodeType::CallExpression:
                return std::formatter<std::string>::format("CallExpression", ctx);
            case kepler::ast::ASTNodeType::CastExpression:
                return std::formatter<std::string>::format("CastExpression", ctx);
            case kepler::ast::ASTNodeType::MathematicalNegationExpression:
                return std::formatter<std::string>::format("MathematicalNegationExpression", ctx);
            case kepler::ast::ASTNodeType::VariableExpression:
                return std::formatter<std::string>::format("VariableExpression", ctx);
            default:
                kepler::log::warning("Missing format implementation for operator type '{}'", static_cast<int>(ast_node_type));
                return std::formatter<std::string>::format(std::format("{}", static_cast<int>(ast_node_type)), ctx);
        }
    }
};
