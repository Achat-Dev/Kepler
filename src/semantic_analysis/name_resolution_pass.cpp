// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/name_resolution_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "ast/struct.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/scope.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include "utils/string_utils.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    NameResolutionPass::NameResolutionPass(DiagnosticSink& diagnostic_sink, SymbolTable& symbol_table, TypeTable& type_table)
        : diagnostic_sink(diagnostic_sink), symbol_table(symbol_table), type_table(type_table) {
        if (builtin_type_symbol_ids.empty()) {
            register_builtin_type_symbol(TypeKind::Void);
            register_builtin_type_symbol(TypeKind::Bool);
            register_builtin_type_symbol(TypeKind::String);
            register_builtin_type_symbol(TypeKind::I8);
            register_builtin_type_symbol(TypeKind::I16);
            register_builtin_type_symbol(TypeKind::I32);
            register_builtin_type_symbol(TypeKind::I64);
            register_builtin_type_symbol(TypeKind::U8);
            register_builtin_type_symbol(TypeKind::U16);
            register_builtin_type_symbol(TypeKind::U32);
            register_builtin_type_symbol(TypeKind::U64);
            register_builtin_type_symbol(TypeKind::F32);
            register_builtin_type_symbol(TypeKind::F64);
        }
    }

    void NameResolutionPass::register_builtin_type_symbol(TypeKind type_kind) {
        KPL_ASSERT_THAT(type_kind != TypeKind::Struct);
        const Symbol* type_symbol = symbol_table.lookup(get_builtin_type_symbol_id(type_kind));
        builtin_type_symbol_ids.emplace(type_symbol->identifier_id, type_symbol->id);
    }

    void NameResolutionPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        // Create user defined types
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            module_id = ast.module_statement->module_id;
            create_struct_symbols_and_types(ast.struct_nodes);
        }

        // Create prototype symbols and struct members (requires all types to exist)
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            module_id = ast.module_statement->module_id;
            create_prototype_symbols(ast.top_level_nodes);
            create_struct_members(ast.struct_nodes);
        }

        // Resolve all remaining names (requires all types and prototypes to exists)
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            module_id = ast.module_statement->module_id;
            resolve_nodes(ast.top_level_nodes);
        }

        // Cleanup state
        module_id = ModuleId::invalid();
    }

    void NameResolutionPass::create_struct_symbols_and_types(const std::vector<std::unique_ptr<Struct>>& struct_nodes) {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (const std::unique_ptr<Struct>& struct_node : struct_nodes) {
            KPL_ASSERT_THAT(struct_node->type_id == TypeId::invalid());
            const auto symbol_id = symbol_table.create_struct(module_id, struct_node->identifier_id);
            if (!symbol_id.has_value()) {
                const Diagnostic& diagnostic = symbol_id.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, struct_node->source_location);
                struct_node->node_type = ASTNodeType::Poison;
                continue;
            }
            const TypeId type_id = type_table.create_struct(struct_node->identifier_id, symbol_id.value());
            Symbol* symbol = symbol_table.lookup(symbol_id.value());
            symbol->type_id = type_id;
            struct_node->type_id = type_id;
        }
    }

    void NameResolutionPass::create_prototype_symbols(const std::vector<std::unique_ptr<ASTNode>>& top_level_nodes) {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (const std::unique_ptr<ASTNode>& node : top_level_nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            switch (node->node_type) {
                case ASTNodeType::Extern: {
                    Extern* ext = static_cast<Extern*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(ext->prototype);
                    const NameResolutionResult resolution_result = create_prototype_symbol(ext->prototype.get(), ext->linkage_type);
                    if (resolution_result.poisoned) {
                        ext->node_type = ASTNodeType::Poison;
                    }
                    break;
                }
                case ASTNodeType::Function: {
                    Function* function = static_cast<Function*>(node.get());
                    KPL_ASSERT_NOT_NULLPTR(function->prototype);
                    const NameResolutionResult resolution_result = create_prototype_symbol(function->prototype.get(), function->linkage_type);
                    if (resolution_result.poisoned) {
                        function->node_type = ASTNodeType::Poison;
                    }
                    break;
                }
                default:
                    KPL_ASSERT_UNREACHABLE("Invalid ast node type of type '{}' on top level during name resolution", node->node_type);
            }
        }
    }

    NameResolutionResult NameResolutionPass::create_prototype_symbol(Prototype* prototype, LinkageType linkage_type) const {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(prototype->return_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);

        // Check if valid return type
        const auto return_type_symbol = resolve_type_identifier_to_type_symbol(prototype->return_type_identifier_id);
        if (!return_type_symbol.has_value()) {
            const Diagnostic& diagnostic = return_type_symbol.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        } else {
            KPL_ASSERT_NOT_NULLPTR(return_type_symbol.value());
            KPL_ASSERT_THAT(return_type_symbol.value()->type_id != TypeId::invalid());
            prototype->return_type_id = return_type_symbol.value()->type_id;
        }

        // Check if valid identifier
        TypeSymbol* object_type_symbol = nullptr;
        StringId method_identifier_id;
        const StringId identifier_id = std::visit(
            [this, prototype, &object_type_symbol, &method_identifier_id](const auto& prototype_identifier) -> StringId {
                using ValueType = std::decay_t<decltype(prototype_identifier)>;
                if constexpr (std::is_same_v<ValueType, StringId>) {
                    KPL_ASSERT_THAT(prototype_identifier != StringId::invalid());
                    const auto identifier_type_symbol = resolve_type_identifier_to_type_symbol(prototype_identifier);
                    if (identifier_type_symbol.has_value()) {
                        const std::string message = std::format("Type name '{}' cannot be used as an identifier",
                            StringPool::get().lookup(prototype_identifier));
                        diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), prototype->identifier_source_location);
                        prototype->node_type = ASTNodeType::Poison;
                        return StringId::invalid();
                    }
                    return prototype_identifier;
                } else if constexpr (std::is_same_v<ValueType, MemberFunctionIdentifier>) {
                    KPL_ASSERT_THAT(prototype_identifier.type_identifier_id != StringId::invalid());
                    KPL_ASSERT_THAT(prototype_identifier.identifier_id != StringId::invalid());
                    // Resolve type part of path (*Type*.Function)
                    const auto type_symbol = resolve_type_identifier_to_type_symbol(prototype_identifier.type_identifier_id);
                    if (!type_symbol.has_value()) {
                        const Diagnostic& diagnostic = type_symbol.error();
                        diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), prototype_identifier.type_source_location);
                        prototype->node_type = ASTNodeType::Poison;
                        return StringId::invalid();
                    }
                    object_type_symbol = type_symbol.value();
                    // Resolve member identifier part of path (Type.*Function*)
                    const auto identifier_type_id = resolve_type_identifier_to_type_symbol(prototype_identifier.identifier_id);
                    if (identifier_type_id.has_value()) {
                        const std::string message = std::format("Type name '{}' cannot be used as a member function identifier",
                            StringPool::get().lookup(prototype_identifier.identifier_id));
                        diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), prototype_identifier.identifier_source_location);
                        prototype->node_type = ASTNodeType::Poison;
                        return StringId::invalid();
                    }
                    method_identifier_id = prototype_identifier.identifier_id;
                    return get_prototype_identifier_id(prototype);
                }
                KPL_ASSERT_UNREACHABLE("Missing visit implementation for PrototypeIdentifier '{}'",
                    typeid(prototype_identifier).name());
            },
            prototype->identifier);
        if (identifier_id == StringId::invalid()) {
            return {.poisoned = true};
        }

        // Check if valid parameters
        std::vector<TypeId> parameter_type_ids;
        std::vector<StringId> parameter_type_identifier_ids;
        parameter_type_ids.reserve(prototype->parameter_data.size());
        parameter_type_identifier_ids.reserve(prototype->parameter_data.size());
        for (ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type_id == TypeId::invalid());
            KPL_ASSERT_THAT(parameter_data.type_identifier_id != StringId::invalid());
            const auto parameter_type_symbol = resolve_type_identifier_to_type_symbol(parameter_data.type_identifier_id);
            if (!parameter_type_symbol.has_value()) {
                const Diagnostic& diagnostic = parameter_type_symbol.error();
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), parameter_data.type_source_location);
                prototype->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
            KPL_ASSERT_NOT_NULLPTR(parameter_type_symbol.value());
            const TypeId parameter_type_id = parameter_type_symbol.value()->type_id;
            KPL_ASSERT_THAT(parameter_type_id != TypeId::invalid());
            parameter_data.type_id = parameter_type_id;
            parameter_type_ids.push_back(parameter_type_id);
            parameter_type_identifier_ids.push_back(parameter_data.type_identifier_id);
        }

        if (object_type_symbol != nullptr && method_identifier_id != StringId::invalid()) {
            const NameResolutionResult operator_signature_nrr = ensure_operator_signatures(prototype,
                object_type_symbol->type_id,
                method_identifier_id,
                parameter_type_ids.size());
            if (operator_signature_nrr.poisoned) {
                prototype->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
        }

        const auto symbol_id = symbol_table.create_prototype(module_id,
            return_type_symbol.value()->type_id,
            identifier_id,
            linkage_type,
            std::move(parameter_type_ids),
            std::move(parameter_type_identifier_ids),
            prototype->is_variadic,
            object_type_symbol == nullptr ? SymbolId::invalid() : object_type_symbol->id);
        if (!symbol_id.has_value()) {
            const Diagnostic& diagnostic = symbol_id.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, prototype->identifier_source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        prototype->symbol_id = symbol_id.value();
        return {.poisoned = false};
    }

    // clang-format off
    NameResolutionResult NameResolutionPass::ensure_operator_signatures(Prototype* prototype,
        TypeId object_type_id,
        StringId method_identifier_id,
        uint32_t parameter_count) const
    {
        // clang-format on
        // Make sure that the signature of the prototype is correct if it's an operator or a cast
        // We do this here even though it has something to do with typechecking because that way
        // we don't even create the prototype symbol, which means that it will never show as the result of a search,
        // which will result in better performance
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(prototype->return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(object_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(method_identifier_id != StringId::invalid());
        if (method_identifier_id == StringPool::get().store("__cast") && prototype->return_type_id != object_type_id) {
            const std::string message = std::format("Type mismatch: A cast must return the type it's defined for (expected '{}', got '{}')",
                *type_table.lookup(object_type_id),
                *type_table.lookup(prototype->return_type_id));
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        const OperatorSignatureData actual_signature{
            .return_type_id = prototype->return_type_id,
            .identifier_id = method_identifier_id,
            .parameter_count = parameter_count,
        };
        ensure_operator_signature(prototype,
            {.return_type_id = object_type_id, .identifier_id = get_operator_name_id(OperatorType::Plus), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = object_type_id, .identifier_id = get_operator_name_id(OperatorType::Minus), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = object_type_id, .identifier_id = get_operator_name_id(OperatorType::Multiplication), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = object_type_id, .identifier_id = get_operator_name_id(OperatorType::Division), .parameter_count = 2},
            actual_signature);
        const TypeId bool_type_id = get_builtin_type_id(TypeKind::Bool);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::LessThan), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::GreaterThan), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::Equals), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::NotEquals), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::LessEquals), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = bool_type_id, .identifier_id = get_operator_name_id(OperatorType::GreaterEquals), .parameter_count = 2},
            actual_signature);
        ensure_operator_signature(prototype,
            {.return_type_id = object_type_id, .identifier_id = StringPool::get().store("__mneg"), .parameter_count = 1},
            actual_signature);

        return {.poisoned = false};
    }

    // clang-format off
    void NameResolutionPass::ensure_operator_signature(Prototype* prototype,
        const OperatorSignatureData& expected_signature,
        const OperatorSignatureData& actual_signature) const
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(prototype);
        KPL_ASSERT_THAT(expected_signature.identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expected_signature.return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(actual_signature.identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(actual_signature.return_type_id != TypeId::invalid());
        if (prototype->node_type == ASTNodeType::Poison) {
            return;
        }
        if (expected_signature.identifier_id != actual_signature.identifier_id) {
            return;
        }

        if (expected_signature.return_type_id != actual_signature.return_type_id) {
            std::string message = "Type mismatch: Operator '" + StringPool::get().lookup(expected_signature.identifier_id) + "' must return ";
            if (expected_signature.return_type_id == get_builtin_type_id(TypeKind::Bool)) {
                message += std::format("'{}', got '{}'",
                    *type_table.lookup(expected_signature.return_type_id),
                    *type_table.lookup(actual_signature.return_type_id));
            } else {
                message += std::format("the type it's defined for (expected '{}', got '{}'",
                    *type_table.lookup(expected_signature.return_type_id),
                    *type_table.lookup(actual_signature.return_type_id));
            }
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return;
        }

        if (expected_signature.parameter_count != actual_signature.parameter_count) {
            const std::string message = std::format("Parameter count mismatch: Operator '{}' expects {} parameters, got {})",
                StringPool::get().lookup(expected_signature.identifier_id),
                expected_signature.parameter_count,
                actual_signature.parameter_count);
            // TODO (improvement): Point the source location at the mismatched parameters
            diagnostic_sink.report(DiagnosticCode::MismatchingOperatorParameterCount, std::move(message), prototype->source_location);
            prototype->node_type = ASTNodeType::Poison;
            return;
        }
    }

    void NameResolutionPass::create_struct_members(const std::vector<std::unique_ptr<Struct>>& struct_nodes) {
        for (const std::unique_ptr<Struct>& struct_node : struct_nodes) {
            KPL_ASSERT_NOT_NULLPTR(struct_node);
            KPL_ASSERT_THAT(struct_node->identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(struct_node->type_id != TypeId::invalid());
            std::vector<StructNodeMember> valid_members;
            for (StructNodeMember& member : struct_node->members) {
                KPL_ASSERT_THAT(member.type_identifier_id != StringId::invalid());
                KPL_ASSERT_THAT(member.type_id == TypeId::invalid());
                // Check if valid type
                const auto type_symbol = resolve_type_identifier_to_type_symbol(member.type_identifier_id);
                if (!type_symbol.has_value()) {
                    const Diagnostic& diagnostic = type_symbol.error();
                    diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), member.type_source_location);
                    struct_node->node_type = ASTNodeType::Poison;
                    continue;
                }
                KPL_ASSERT_NOT_NULLPTR(type_symbol.value());
                KPL_ASSERT_THAT(type_symbol.value()->type_id != TypeId::invalid());
                member.type_id = type_symbol.value()->type_id;
                // Check if valid identifier
                const auto identifier_type_symbol = resolve_type_identifier_to_type_symbol(member.identifier_id);
                if (identifier_type_symbol.has_value()) {
                    const std::string message = std::format("Type name '{}' cannot be used as an identifier",
                        StringPool::get().lookup(member.identifier_id));
                    diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), member.identifier_source_location);
                    struct_node->node_type = ASTNodeType::Poison;
                    continue;
                }
                // Check if duplicate identifier
                bool is_duplicate_member = false;
                for (const StructNodeMember& valid_member : valid_members) {
                    if (member.identifier_id == valid_member.identifier_id) {
                        const std::string message = std::format("Member with identifier '{}' already exists in struct '{}'",
                            StringPool::get().lookup(member.identifier_id),
                            StringPool::get().lookup(struct_node->identifier_id));
                        diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), member.identifier_source_location);
                        struct_node->node_type = ASTNodeType::Poison;
                        is_duplicate_member = true;
                        break;
                    }
                }
                if (is_duplicate_member) {
                    continue;
                }

                valid_members.push_back(member);
            }
            type_table.create_struct_members(struct_node->type_id, valid_members);
        }
    }

    NameResolutionResult NameResolutionPass::resolve_nodes(std::vector<std::unique_ptr<ASTNode>>& nodes) const {
        bool poisoned = false;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            const NameResolutionResult resolution_result = resolve_node(node.get());
            if (resolution_result.poisoned) {
                poisoned = true;
            }
        }
        return {.poisoned = poisoned};
    }

    NameResolutionResult NameResolutionPass::resolve_node(ASTNode* node) const {
        KPL_ASSERT_NOT_NULLPTR(node);
        switch (node->node_type) {
            case ASTNodeType::Struct:
            case ASTNodeType::Prototype:
            case ASTNodeType::ImportStatement:
            case ASTNodeType::ModuleStatement:
                KPL_ASSERT_UNREACHABLE("Cannot do name resolution for a node of type '{}' as part of the top level nodes", node->node_type);

            // Literals cannot be analysed for symbols
            case ASTNodeType::BooleanLiteralExpression:
            case ASTNodeType::FloatingPointLiteralExpression:
            case ASTNodeType::IntegerLiteralExpression:
            case ASTNodeType::StringLiteralExpression:
                return {.poisoned = false};

            case ASTNodeType::Poison:
                return {.poisoned = true};
            case ASTNodeType::Extern:
                resolve_extern(static_cast<Extern*>(node));
                return {.poisoned = false};
            case ASTNodeType::Function:
                resolve_function(static_cast<Function*>(node));
                return {.poisoned = false};
            case ASTNodeType::AssignmentStatement:
                return resolve_assignment_statement(static_cast<AssignmentStatement*>(node), SymbolId::invalid());
            case ASTNodeType::ForStatement:
                return resolve_for_statement(static_cast<ForStatement*>(node));
            case ASTNodeType::IfStatement:
                return resolve_if_statement(static_cast<IfStatement*>(node));
            case ASTNodeType::ReturnStatement:
                return resolve_return_statement(static_cast<ReturnStatement*>(node));
            case ASTNodeType::VariableDefinitionStatement:
                return resolve_variable_definition_statement(static_cast<VariableDefinitionStatement*>(node));
            case ASTNodeType::BinaryExpression:
                return resolve_binary_expression(static_cast<BinaryExpression*>(node));
            case ASTNodeType::CallExpression:
                return resolve_call_expression(static_cast<CallExpression*>(node));
            case ASTNodeType::MathematicalNegationExpression:
                return resolve_mathematical_negation_expression(static_cast<MathematicalNegationExpression*>(node));
            case ASTNodeType::MemberExpression:
                return resolve_member_expression(static_cast<MemberExpression*>(node));
            case ASTNodeType::ObjectInitializerExpression:
                return resolve_object_initializer_expression(static_cast<ObjectInitializerExpression*>(node));
            case ASTNodeType::VariableExpression:
                return resolve_variable_expression(static_cast<VariableExpression*>(node), SymbolId::invalid());
        }

        KPL_ASSERT_UNREACHABLE("Missing name resolution implementation for node of type '{}'", node->node_type);
    }

    void NameResolutionPass::resolve_extern(Extern* ext) const {
        KPL_ASSERT_NOT_NULLPTR(ext);
        KPL_ASSERT_NOT_NULLPTR(ext->prototype);
        KPL_ASSERT_THAT(ext->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Function);
        const NameResolutionResult prototype_result = resolve_prototype(ext->prototype.get());
        symbol_table.close_scope(module_id);
        if (prototype_result.poisoned) {
            ext->node_type = ASTNodeType::Poison;
        }
    }

    void NameResolutionPass::resolve_function(Function* function) const {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Function);
        resolve_prototype(function->prototype.get());
        resolve_nodes(function->body.nodes);
        symbol_table.close_scope(module_id);
    }

    NameResolutionResult NameResolutionPass::resolve_prototype(Prototype* prototype) const {
        KPL_ASSERT_NOT_NULLPTR(prototype);
        // Normally this assert would be there,
        // but because function overloading isn't implemented yet, prototypes poison themselves when trying to overload a function
        // KPL_ASSERT_THAT(prototype->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        for (ParameterData& parameter_data : prototype->parameter_data) {
            KPL_ASSERT_THAT(parameter_data.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(parameter_data.identifier_id != StringId::invalid());
            KPL_ASSERT_THAT(parameter_data.symbol_id == SymbolId::invalid());
            const auto identifier_type_symbol = resolve_type_identifier_to_type_symbol(parameter_data.identifier_id);
            if (identifier_type_symbol.has_value()) {
                const std::string message = std::format("Type name '{}' cannot be used as an identifier",
                    StringPool::get().lookup(parameter_data.identifier_id));
                diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), parameter_data.identifier_source_location);
                prototype->node_type = ASTNodeType::Poison;
                continue;
            }

            const auto symbol_id = symbol_table.create_variable(module_id, parameter_data.type_id, parameter_data.identifier_id);
            if (!symbol_id) {
                const Diagnostic& diagnostic = symbol_id.error();
                diagnostic_sink.report(diagnostic.code, diagnostic.message, parameter_data.identifier_source_location);
                prototype->node_type = ASTNodeType::Poison;
            } else {
                parameter_data.symbol_id = *symbol_id;
            }
        }
        return {.poisoned = prototype->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_assignment_statement(AssignmentStatement* statement, SymbolId variable_symbol_id) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        const NameResolutionResult assignment_target_nrr = std::visit(
            [this, variable_symbol_id](auto& expression) -> NameResolutionResult {
                KPL_ASSERT_NOT_NULLPTR(expression);
                KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
                using ValueType = std::decay_t<decltype(expression)>;
                if constexpr (std::is_same_v<ValueType, std::unique_ptr<VariableExpression>>) {
                    VariableExpression* variable_expression = static_cast<VariableExpression*>(expression.get());
                    return resolve_variable_expression(variable_expression, variable_symbol_id);
                } else if constexpr (std::is_same_v<ValueType, std::unique_ptr<MemberExpression>>) {
                    return resolve_node(expression.get());
                }
                KPL_ASSERT_UNREACHABLE("Missing visit implementation for AssignmentTargetExpression '{}'", typeid(expression).name());
            },
            statement->assignment_target_expression);
        const NameResolutionResult value_nrr = resolve_node(statement->value_expression.get());
        if (assignment_target_nrr.poisoned || value_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_for_statement(ForStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult definition_nrr = resolve_variable_definition_statement(statement->loop_variable_definition.get());
        const NameResolutionResult end_nrr = resolve_node(statement->end_value.get());
        NameResolutionResult step_nrr{.poisoned = false};
        if (statement->step_value != nullptr) {
            step_nrr = resolve_node(statement->step_value.get());
        }
        const NameResolutionResult body_nrr = resolve_nodes(statement->body.nodes);
        symbol_table.close_scope(module_id);

        if (definition_nrr.poisoned || end_nrr.poisoned || step_nrr.poisoned || body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_if_statement(IfStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const NameResolutionResult condition_nrr = resolve_node(statement->condition.get());

        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult if_body_nrr = resolve_nodes(statement->if_body.nodes);
        symbol_table.close_scope(module_id);

        symbol_table.open_scope(module_id, ScopeType::Block);
        const NameResolutionResult else_body_nrr = resolve_nodes(statement->else_body.nodes);
        symbol_table.close_scope(module_id);

        if (condition_nrr.poisoned || if_body_nrr.poisoned || else_body_nrr.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_return_statement(ReturnStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        if (statement->expression == nullptr) {
            return {.poisoned = false};
        }

        const NameResolutionResult resolution_result = resolve_node(statement->expression.get());
        if (resolution_result.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_variable_definition_statement(VariableDefinitionStatement* statement) const {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_THAT(statement->type_id == TypeId::invalid());
        KPL_ASSERT_THAT(statement->type_identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(statement->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const AssignmentTargetExpression& assignment_target_expression = statement->assignment_statement->assignment_target_expression;
        KPL_ASSERT_THAT(std::holds_alternative<std::unique_ptr<VariableExpression>>(assignment_target_expression));
        const std::unique_ptr<VariableExpression>& variable_expression = std::get<std::unique_ptr<VariableExpression>>(assignment_target_expression);
        const auto identifier_type_symbol = resolve_type_identifier_to_type_symbol(statement->identifier_id);
        if (identifier_type_symbol.has_value()) {
            const std::string message = std::format("Type name '{}' cannot be used as an identifier", StringPool::get().lookup(statement->identifier_id));
            diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), variable_expression->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        const auto type_symbol = resolve_type_identifier_to_type_symbol(statement->type_identifier_id);
        if (!type_symbol.has_value()) {
            const Diagnostic& diagnostic = type_symbol.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        KPL_ASSERT_NOT_NULLPTR(type_symbol.value());
        const TypeId type_id = type_symbol.value()->type_id;
        KPL_ASSERT_THAT(type_id != TypeId::invalid());
        statement->type_id = type_id;

        const auto symbol_id = symbol_table.create_variable(module_id, type_id, statement->identifier_id);
        if (!symbol_id) {
            const Diagnostic& diagnostic = symbol_id.error();
            diagnostic_sink.report(diagnostic.code, diagnostic.message, variable_expression->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        const NameResolutionResult resolution_result = resolve_assignment_statement(statement->assignment_statement.get(), symbol_id.value());
        if (resolution_result.poisoned) {
            statement->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_binary_expression(BinaryExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const NameResolutionResult lhs_nrr = resolve_node(expression->lhs.get());
        const NameResolutionResult rhs_nrr = resolve_node(expression->rhs.get());
        if (lhs_nrr.poisoned || rhs_nrr.poisoned) {
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_call_expression(CallExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(!expression->identifier_path.identifier_parts.empty());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());

        // Right now this is fine because there are no member method calls
        // This won't work later on when the actual identifier path has to be resolved in case the method of another member is called
        const std::string callee_identifier = get_full_identifier_from_path(expression->identifier_path);

        // If the identifier resolves to a type, it's a cast
        const StringId callee_identifier_id = StringPool::get().store(callee_identifier);
        const auto callee_type_symbol = resolve_type_identifier_to_type_symbol(callee_identifier_id);
        const bool is_cast = callee_type_symbol.has_value();
        expression->cast_data.is_cast = is_cast;

        std::expected<std::vector<Symbol*>, Diagnostic> candidate_symbols;
        if (expression->module_path.identifier_parts.empty()) {
            if (is_cast) {
                candidate_symbols = symbol_table.find_symbol_of_kind(module_id, callee_identifier_id, SymbolKind::Type);
            } else {
                candidate_symbols = symbol_table.find_symbol_of_kind(module_id, callee_identifier_id, SymbolKind::Prototype);
            }
        } else {
            if (is_cast) {
                candidate_symbols = symbol_table.find_symbol_of_kind(module_id, expression->module_path, callee_identifier_id, SymbolKind::Type);
            } else {
                candidate_symbols = symbol_table.find_symbol_of_kind(module_id, expression->module_path, callee_identifier_id, SymbolKind::Prototype);
            }
        }

        if (!candidate_symbols.has_value()) {
            const Diagnostic diagnostic = candidate_symbols.error();
            if (diagnostic.code == DiagnosticCode::UnknownModule || diagnostic.code == DiagnosticCode::AmbiguousModulePath) {
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->module_path.source_location);
            } else {
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            }
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        if (candidate_symbols.value().empty()) {
            const std::string& identifier = StringPool::get().lookup(callee_identifier_id);
            diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::format("Call to unknown function '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        std::vector<SymbolId> candidate_symbol_ids;
        for (const Symbol* found_symbol : candidate_symbols.value()) {
            candidate_symbol_ids.push_back(found_symbol->id);
        }
        expression->candidate_symbol_ids = std::move(candidate_symbol_ids);

        for (const std::unique_ptr<Expression>& arg : expression->args) {
            KPL_ASSERT_NOT_NULLPTR(arg);
            const NameResolutionResult resolution_result = resolve_node(arg.get());
            if (resolution_result.poisoned) {
                expression->node_type = ASTNodeType::Poison;
            }
        }
        return {.poisoned = expression->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_mathematical_negation_expression(MathematicalNegationExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const NameResolutionResult resolution_result = resolve_node(expression->expression.get());
        if (resolution_result.poisoned) {
            expression->node_type = ASTNodeType::Poison;
        }
        return resolution_result;
    }

    NameResolutionResult NameResolutionPass::resolve_member_expression(MemberExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->object_symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(expression->access_data.empty());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const std::vector<IdentifierPathPart> member_path_parts = expression->member_path.identifier_parts;
        KPL_ASSERT_THAT(member_path_parts.size() > 1);
        const IdentifierPathPart& first_path_part = member_path_parts[0];
        KPL_ASSERT_THAT(first_path_part.identifier_id != StringId::invalid());
        const auto found_symbols = symbol_table.find_symbol_of_kind(module_id, first_path_part.identifier_id, SymbolKind::Variable);
        if (!found_symbols.has_value()) {
            const Diagnostic& diagnostic = found_symbols.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), first_path_part.source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        if (found_symbols.value().empty()) {
            const std::string& identifier = StringPool::get().lookup(first_path_part.identifier_id);
            diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::format("Unknown symbol '{}'", identifier), first_path_part.source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        KPL_ASSERT_THAT(found_symbols.value().size() == 1);
        const Symbol* object_symbol = found_symbols.value()[0];
        KPL_ASSERT_THAT(object_symbol->type_id != TypeId::invalid());
        const Type* object_type = type_table.lookup(object_symbol->type_id);
        if (object_type->type_kind != TypeKind::Struct) {
            const std::string message = std::format("Symbol '{}' is of type '{}', which doesn't have any members",
                StringPool::get().lookup(first_path_part.identifier_id),
                *object_type);
            diagnostic_sink.report(DiagnosticCode::InvalidMemberAccess, std::move(message), first_path_part.source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        expression->object_symbol_id = object_symbol->id;

        const StructType* struct_type = static_cast<const StructType*>(object_type);
        TypeId member_type_id;
        std::vector<MemberAccessData> member_access_data;
        for (size_t i = 1; i < member_path_parts.size(); i++) {
            const IdentifierPathPart& identifier_path_part = member_path_parts[i];
            KPL_ASSERT_THAT(identifier_path_part.identifier_id != StringId::invalid());
            const StructTypeMember* member = struct_type->find_member(identifier_path_part.identifier_id);
            if (member == nullptr) {
                const std::string message = std::format("Unknown member '{}' on type '{}'",
                    StringPool::get().lookup(identifier_path_part.identifier_id),
                    *object_type);
                diagnostic_sink.report(DiagnosticCode::UnknownMember, std::move(message), identifier_path_part.source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
            KPL_ASSERT_THAT(member->type_id != TypeId::invalid());
            member_access_data.push_back({
                .struct_type_id = struct_type->id,
                .member_index = struct_type->get_member_index(identifier_path_part.identifier_id),
            });
            member_type_id = member->type_id;

            Type* member_type = type_table.lookup(member->type_id);
            KPL_ASSERT_THAT(member_type->type_kind != TypeKind::Void);
            if (member_type->type_kind == TypeKind::Struct) {
                struct_type = static_cast<StructType*>(member_type);
            } else if (i != member_path_parts.size() - 1) {
                const std::string message = std::format("Member '{}' is of type '{}', which doesn't have any members",
                    StringPool::get().lookup(identifier_path_part.identifier_id),
                    *member_type);
                diagnostic_sink.report(DiagnosticCode::InvalidMemberAccess, std::move(message), identifier_path_part.source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
        }

        KPL_ASSERT_THAT(member_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(!member_access_data.empty());
        expression->member_type_id = member_type_id;
        expression->access_data = std::move(member_access_data);
        return {.poisoned = false};
    }

    NameResolutionResult NameResolutionPass::resolve_object_initializer_expression(ObjectInitializerExpression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        if (expression->type_identifier_id != StringId::invalid()) {
            const auto type_symbol = resolve_type_identifier_to_type_symbol(expression->type_identifier_id);
            if (!type_symbol.has_value()) {
                const Diagnostic& diagnostic = type_symbol.error();
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->type_source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.poisoned = true};
            }
            KPL_ASSERT_NOT_NULLPTR(type_symbol.value());
            KPL_ASSERT_THAT(type_symbol.value()->type_id != TypeId::invalid());
            expression->type_id = type_symbol.value()->type_id;
        }

        for (const MemberInitializerData& member_initializer_data : expression->member_initializers) {
            KPL_ASSERT_NOT_NULLPTR(member_initializer_data.value_expression);
            const NameResolutionResult resolution_result = resolve_node(member_initializer_data.value_expression.get());
            if (resolution_result.poisoned) {
                expression->node_type = ASTNodeType::Poison;
            }
        }
        return {.poisoned = expression->node_type == ASTNodeType::Poison};
    }

    NameResolutionResult NameResolutionPass::resolve_variable_expression(VariableExpression* expression, SymbolId variable_symbol_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(expression->identifier_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        const auto identifier_type_symbol = resolve_type_identifier_to_type_symbol(expression->identifier_id);
        if (identifier_type_symbol.has_value()) {
            const std::string message = std::format("Type name '{}' cannot be used as an identifier",
                StringPool::get().lookup(expression->identifier_id));
            diagnostic_sink.report(DiagnosticCode::InvalidIdentifier, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }

        // If the symbol is already known (through a variable definition), we don't have to search for it
        // This avoids ambiguous symbol diagnostics for variable definitions
        if (variable_symbol_id != SymbolId::invalid()) {
            expression->symbol_id = variable_symbol_id;
            return {.poisoned = false};
        }

        const auto found_symbols = symbol_table.find_symbol_of_kind(module_id, expression->identifier_id, SymbolKind::Variable);
        if (!found_symbols.has_value()) {
            const Diagnostic diagnostic = found_symbols.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        if (found_symbols.value().empty()) {
            const std::string& identifier = StringPool::get().lookup(expression->identifier_id);
            diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::format("Unknown symbol '{}'", identifier), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        KPL_ASSERT_THAT(found_symbols.value().size() == 1);
        const Symbol* variable_symbol = found_symbols.value()[0];
        const SymbolKind variable_symbol_kind = variable_symbol->get_symbol_kind();
        if (variable_symbol_kind != SymbolKind::Variable) {
            std::string variable_symbol_kind_identifier = std::format("{}", variable_symbol_kind);
            str_initial_to_lower_case(variable_symbol_kind_identifier);
            const std::string& identifier = StringPool::get().lookup(expression->identifier_id);
            const std::string message = std::format("Symbol '{}' is used as a variable, but it's a {}",
                identifier,
                std::move(variable_symbol_kind_identifier));
            diagnostic_sink.report(DiagnosticCode::InvalidSymbolUsage, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.poisoned = true};
        }
        expression->symbol_id = variable_symbol->id;
        return {.poisoned = false};
    }

    std::expected<TypeSymbol*, Diagnostic> NameResolutionPass::resolve_type_identifier_to_type_symbol(StringId type_identifier_id) const {
        KPL_ASSERT_THAT(module_id != ModuleId::invalid());
        KPL_ASSERT_THAT(type_identifier_id != StringId::invalid());
        const auto it = builtin_type_symbol_ids.find(type_identifier_id);
        if (it != builtin_type_symbol_ids.end()) {
            KPL_ASSERT_THAT(it->second != SymbolId::invalid());
            Symbol* symbol = symbol_table.lookup(it->second);
            KPL_ASSERT_THAT(symbol->get_symbol_kind() == SymbolKind::Type);
            return static_cast<TypeSymbol*>(symbol);
        }

        const auto found_symbols = symbol_table.find_symbol_of_kind(module_id, type_identifier_id, SymbolKind::Type);
        if (!found_symbols.has_value()) {
            return std::unexpected(std::move(found_symbols.error()));
        }
        if (found_symbols.value().empty()) {
            const std::string& type_name = StringPool::get().lookup(type_identifier_id);
            return std::unexpected(Diagnostic{.code = DiagnosticCode::UnknownType, .message = std::format("Unknown type '{}'", type_name)});
        }
        KPL_ASSERT_THAT(found_symbols.value().size() == 1); // If multiple types were found it would have been reported as a diagnostic
        Symbol* type_symbol = found_symbols.value()[0];
        const SymbolKind type_symbol_kind = type_symbol->get_symbol_kind();
        if (type_symbol_kind != SymbolKind::Type) {
            const std::string& type_identifier = StringPool::get().lookup(type_identifier_id);
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::UnknownType,
                .message = std::format("{} '{}' cannot be used as a type", type_symbol_kind, type_identifier),
            });
        }
        KPL_ASSERT_THAT(type_symbol->type_id != TypeId::invalid());
        return static_cast<TypeSymbol*>(type_symbol);
    }

}
