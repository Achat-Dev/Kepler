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
#include <llvm/IR/Function.h>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    enum class ASTNodeKind {
        Poison,
        Extern,
        Struct,
        Function,
        Prototype,
        AssignmentStatement,
        ForStatement,
        IfStatement,
        ImportStatement,
        ModuleStatement,
        ReturnStatement,
        VariableDefinitionStatement,
        BooleanLiteralExpression,
        FloatingPointLiteralExpression,
        IntegerLiteralExpression,
        StringLiteralExpression,
        BinaryExpression,
        CallExpression,
        MathematicalNegationExpression,
        MemberExpression,
        ObjectInitializerExpression,
        VariableExpression,
    };

    enum class LinkageKind {
        Internal,
        External,
        Export,
    };

    struct ASTNode {
        ASTNodeKind node_kind;
        SourceLocation source_location;

        ASTNode(ASTNodeKind node_kind, SourceLocation source_location)
            : node_kind(node_kind), source_location(std::move(source_location)) {}
        virtual ~ASTNode() = default;
    };

    struct ExportableNode : ASTNode {
        LinkageKind linkage_kind;
        ExportableNode(ASTNodeKind node_kind, LinkageKind linkage_kind, SourceLocation source_location)
            : ASTNode(node_kind, std::move(source_location)), linkage_kind(linkage_kind) {}
    };

    struct NodeBody {
        std::vector<ASTNode*> nodes;
        bool contains_return = false;
    };

    llvm::Function::LinkageTypes get_llvm_linkage_kind(LinkageKind linkage_kind);

}

template <>
struct std::formatter<kepler::ASTNodeKind> : std::formatter<std::string> {
    auto format(const kepler::ASTNodeKind& node_kind, std::format_context& ctx) const {
        switch (node_kind) {
            case kepler::ASTNodeKind::Poison:
                return std::formatter<std::string>::format("Poison", ctx);
            case kepler::ASTNodeKind::Extern:
                return std::formatter<std::string>::format("Extern", ctx);
            case kepler::ASTNodeKind::Struct:
                return std::formatter<std::string>::format("Struct", ctx);
            case kepler::ASTNodeKind::Function:
                return std::formatter<std::string>::format("Function", ctx);
            case kepler::ASTNodeKind::Prototype:
                return std::formatter<std::string>::format("Prototype", ctx);
            case kepler::ASTNodeKind::AssignmentStatement:
                return std::formatter<std::string>::format("AssignmentStatement", ctx);
            case kepler::ASTNodeKind::ForStatement:
                return std::formatter<std::string>::format("ForStatement", ctx);
            case kepler::ASTNodeKind::IfStatement:
                return std::formatter<std::string>::format("IfStatement", ctx);
            case kepler::ASTNodeKind::ImportStatement:
                return std::formatter<std::string>::format("ImportStatement", ctx);
            case kepler::ASTNodeKind::ModuleStatement:
                return std::formatter<std::string>::format("ModuleStatement", ctx);
            case kepler::ASTNodeKind::ReturnStatement:
                return std::formatter<std::string>::format("ReturnStatement", ctx);
            case kepler::ASTNodeKind::VariableDefinitionStatement:
                return std::formatter<std::string>::format("VariableDefinitionStatement", ctx);
            case kepler::ASTNodeKind::BooleanLiteralExpression:
                return std::formatter<std::string>::format("BooleanLiteralExpression", ctx);
            case kepler::ASTNodeKind::FloatingPointLiteralExpression:
                return std::formatter<std::string>::format("FloatingPointLiteralExpression", ctx);
            case kepler::ASTNodeKind::IntegerLiteralExpression:
                return std::formatter<std::string>::format("IntegerLiteralExpression", ctx);
            case kepler::ASTNodeKind::StringLiteralExpression:
                return std::formatter<std::string>::format("StringLiteralExpression", ctx);
            case kepler::ASTNodeKind::BinaryExpression:
                return std::formatter<std::string>::format("BinaryExpression", ctx);
            case kepler::ASTNodeKind::CallExpression:
                return std::formatter<std::string>::format("CallExpression", ctx);
            case kepler::ASTNodeKind::MathematicalNegationExpression:
                return std::formatter<std::string>::format("MathematicalNegationExpression", ctx);
            case kepler::ASTNodeKind::MemberExpression:
                return std::formatter<std::string>::format("MemberExpression", ctx);
            case kepler::ASTNodeKind::ObjectInitializerExpression:
                return std::formatter<std::string>::format("ObjectInitializerExpression", ctx);
            case kepler::ASTNodeKind::VariableExpression:
                return std::formatter<std::string>::format("VariableExpression", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for ast node kind '{}'", static_cast<int>(node_kind));
    }
};

template <>
struct std::formatter<kepler::LinkageKind> : std::formatter<std::string> {
    auto format(const kepler::LinkageKind& linkage_kind, std::format_context& ctx) const {
        switch (linkage_kind) {
            case kepler::LinkageKind::Internal:
                return std::formatter<std::string>::format("Internal", ctx);
            case kepler::LinkageKind::External:
                return std::formatter<std::string>::format("External", ctx);
            case kepler::LinkageKind::Export:
                return std::formatter<std::string>::format("Export (external)", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implementation for linkage kind '{}'", static_cast<int>(linkage_kind));
    }
};
