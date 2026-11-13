// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_check_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/module.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <llvm/ADT/APInt.h>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    void TypeCheckPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        for (const AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(ast.module_statement->module_id != ModuleId::invalid());
            module_id = ast.module_statement->module_id;
            current_function_return_type_id = TypeId::invalid();
            typecheck_nodes(ast.top_level_nodes);
        }
    }

    bool TypeCheckPass::is_boolean_operator(OperatorType type) const {
        switch (type) {
            case OperatorType::Plus:
            case OperatorType::Minus:
            case OperatorType::Multiplication:
            case OperatorType::Division:
                return false;

            case OperatorType::LessThan:
            case OperatorType::GreaterThan:
            case OperatorType::Equals:
            case OperatorType::NotEquals:
            case OperatorType::LessEquals:
            case OperatorType::GreaterEquals:
                return true;
        }

        KPL_ASSERT_UNREACHABLE("Missing boolean operator check implementation for operator type '{}'", type);
    }

    bool TypeCheckPass::is_number_literal_expression(const Expression* expression) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        return expression->node_type == ASTNodeType::IntegerLiteralExpression || expression->node_type == ASTNodeType::FloatingPointLiteralExpression;
    }

    TypeCheckResult TypeCheckPass::typecheck_nodes(const std::vector<ASTNode*>& nodes) {
        TypeCheckResult::Status result_status = TypeCheckResult::Status::RequestFulfilled;
        for (ASTNode* node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            const TypeCheckResult typecheck_result = typecheck_node(node, TypeId::invalid());
            if (typecheck_result.is_poisoned()) {
                result_status = TypeCheckResult::Status::PoisonedWithDiagnostic;
            }
        }
        return {.status = result_status, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_node(ASTNode* node, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(node);
        switch (node->node_type) {
            case ASTNodeType::Struct:
            case ASTNodeType::Prototype:
            case ASTNodeType::ImportStatement:
            case ASTNodeType::ModuleStatement:
                KPL_ASSERT_UNREACHABLE("Cannot typecheck a node of type '{}' as part of the top level nodes", node->node_type);

            case ASTNodeType::Poison:
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            case ASTNodeType::Extern:
                return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
            case ASTNodeType::Function:
                typecheck_function(static_cast<Function*>(node));
                return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
            case ASTNodeType::AssignmentStatement:
                return typecheck_assignment_statement(static_cast<AssignmentStatement*>(node));
            case ASTNodeType::ForStatement:
                return typecheck_for_statement(static_cast<ForStatement*>(node));
            case ASTNodeType::IfStatement:
                return typecheck_if_statement(static_cast<IfStatement*>(node));
            case ASTNodeType::ReturnStatement:
                return typecheck_return_statement(static_cast<ReturnStatement*>(node));
            case ASTNodeType::VariableDefinitionStatement:
                return typecheck_variable_definition_statement(static_cast<VariableDefinitionStatement*>(node));
            case ASTNodeType::BooleanLiteralExpression:
                return typecheck_boolean_literal_expression(static_cast<BooleanLiteralExpression*>(node), requested_type_id);
            case ASTNodeType::FloatingPointLiteralExpression:
                return typecheck_floating_point_literal_expression(static_cast<FloatingPointLiteralExpression*>(node), requested_type_id);
            case ASTNodeType::IntegerLiteralExpression:
                return typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(node), requested_type_id, false);
            case ASTNodeType::StringLiteralExpression:
                return typecheck_string_literal_expression(static_cast<StringLiteralExpression*>(node), requested_type_id);
            case ASTNodeType::BinaryExpression:
                return typecheck_binary_expression(static_cast<BinaryExpression*>(node), requested_type_id);
            case ASTNodeType::CallExpression:
                return typecheck_call_expression(static_cast<CallExpression*>(node), requested_type_id);
            case ASTNodeType::MathematicalNegationExpression:
                return typecheck_mathematical_negation_expression(static_cast<MathematicalNegationExpression*>(node), requested_type_id);
            case ASTNodeType::MemberExpression:
                return typecheck_member_expression(static_cast<MemberExpression*>(node), requested_type_id);
            case ASTNodeType::ObjectInitializerExpression:
                return typecheck_object_initializer_expression(static_cast<ObjectInitializerExpression*>(node), requested_type_id);
            case ASTNodeType::VariableExpression:
                return typecheck_variable_expression(static_cast<VariableExpression*>(node), requested_type_id);
        }
        KPL_ASSERT_UNREACHABLE("Missing typecheck implementation for node type '{}'", node->node_type);
    }

    void TypeCheckPass::typecheck_function(Function* function) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_THAT(function->prototype->return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(current_function_return_type_id == TypeId::invalid());
        current_function_return_type_id = function->prototype->return_type_id;
        // Typecheck the body without poisoning the function so that the unpoisoned part of the body can still be accessed by later passes
        typecheck_nodes(function->body.nodes);
        current_function_return_type_id = TypeId::invalid();
    }

    TypeCheckResult TypeCheckPass::typecheck_assignment_statement(AssignmentStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        const TypeId type_id = std::visit(
            [this](const auto& expression) -> TypeId {
                KPL_ASSERT_NOT_NULLPTR(expression);
                using ValueType = std::decay_t<decltype(expression)>;
                if constexpr (std::is_same_v<ValueType, MemberExpression*>) {
                    KPL_ASSERT_THAT(expression->member_type_id != TypeId::invalid());
                    return expression->member_type_id;
                } else if constexpr (std::is_same_v<ValueType, VariableExpression*>) {
                    KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
                    const Symbol* variable_symbol = symbol_table.lookup(expression->symbol_id);
                    KPL_ASSERT_THAT(variable_symbol->get_symbol_kind() == SymbolKind::Variable);
                    KPL_ASSERT_THAT(variable_symbol->type_id != TypeId::invalid());
                    return variable_symbol->type_id;
                }
                KPL_ASSERT_UNREACHABLE("Missing visit implementation for AssignmentTargetExpression '{}'", typeid(expression).name());
            },
            statement->assignment_target_expression);
        KPL_ASSERT_THAT(type_id != TypeId::invalid());

        const TypeCheckResult typecheck_result = typecheck_node(statement->value_expression, type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* value_type = type_table.lookup(typecheck_result.type_id);
            const Type* variable_type = type_table.lookup(type_id);
            const std::string message = std::format("Type mismatch: Cannot assign a value of type '{}' to a variable of type '{}'",
                *value_type,
                *variable_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_for_statement(ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        const TypeId variable_type_id = statement->loop_variable_definition->type_id;
        KPL_ASSERT_THAT(variable_type_id != TypeId::invalid());
        const Type* variable_type = type_table.lookup(variable_type_id);
        if (!is_integer_type(variable_type)) {
            const std::string message = std::format("Loop variable of a for statement has to be an integer type, got '{}'", *variable_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->loop_variable_definition->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        const TypeCheckResult variable_tcr = typecheck_variable_definition_statement(statement->loop_variable_definition);
        KPL_ASSERT_THAT(variable_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (variable_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }
        KPL_ASSERT_THAT(variable_tcr.type_id != TypeId::invalid());

        const TypeCheckResult end_tcr = typecheck_node(statement->end_value, variable_type_id);
        if (end_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }
        KPL_ASSERT_THAT(end_tcr.type_id != TypeId::invalid());
        if (end_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* end_value_type = type_table.lookup(end_tcr.type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *variable_type, *end_value_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->end_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        // Default value for the typecheck result of the step_value in case the step doesn't exist
        TypeCheckResult step_tcr{.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
        if (statement->step_value != nullptr) {
            step_tcr = typecheck_node(statement->step_value, variable_type_id);
            if (step_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                return typecheck_body_and_poison_for_statement(statement);
            }
            KPL_ASSERT_THAT(step_tcr.type_id != TypeId::invalid());
            if (step_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                const Type* step_value_type = type_table.lookup(step_tcr.type_id);
                const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *variable_type, *step_value_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->step_value->source_location);
                return typecheck_body_and_poison_for_statement(statement);
            }
        }

        const TypeCheckResult body_tcr = typecheck_nodes(statement->body.nodes);
        if (variable_tcr.is_poisoned() || end_tcr.is_poisoned() || step_tcr.is_poisoned() || body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_body_and_poison_for_statement(ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        statement->node_type = ASTNodeType::Poison;
        typecheck_nodes(statement->body.nodes);
        return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_if_statement(IfStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        const TypeCheckResult condition_tcr = typecheck_node(statement->condition, get_builtin_type_id(TypeKind::Bool));
        if (condition_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            KPL_ASSERT_THAT(condition_tcr.type_id != TypeId::invalid());
            const Type* condition_type = type_table.lookup(condition_tcr.type_id);
            const std::string message = std::format("Condition of an IfStatement needs to be of type bool, got '{}'", *condition_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->condition->source_location);
        }

        const TypeCheckResult if_body_tcr = typecheck_nodes(statement->if_body.nodes);
        const TypeCheckResult else_body_tcr = typecheck_nodes(statement->else_body.nodes);
        if (condition_tcr.is_poisoned() || if_body_tcr.is_poisoned() || else_body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_return_statement(ReturnStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(current_function_return_type_id != TypeId::invalid());

        if (current_function_return_type_id == get_builtin_type_id(TypeKind::Void)) {
            if (statement->expression != nullptr) {
                diagnostic_sink.report(DiagnosticCode::InvalidReturn, "Cannot return a value from void function", statement->source_location);
                statement->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
        }

        if (statement->expression == nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn,
                "Expected expression after 'return' (non-void function needs a return value)",
                statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        TypeCheckResult typecheck_result = typecheck_node(statement->expression, current_function_return_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* expression_type = type_table.lookup(typecheck_result.type_id);
            const Type* function_return_type = type_table.lookup(current_function_return_type_id);
            const std::string message = std::format("Type mismatch: Cannot return a value of type '{}' from a function with a return type of '{}'",
                *expression_type,
                *function_return_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_definition_statement(VariableDefinitionStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        KPL_ASSERT_THAT(statement->type_id != TypeId::invalid());
        if (statement->type_id == get_builtin_type_id(TypeKind::Void)) {
            diagnostic_sink.report(DiagnosticCode::InvalidVariableType, "Cannot create a variable of type 'void'", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        const TypeCheckResult typecheck_result = typecheck_assignment_statement(statement->assignment_statement);
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = statement->type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const TypeId bool_type_id = get_builtin_type_id(TypeKind::Bool);
        if (requested_type_id == bool_type_id || requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = bool_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = bool_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const TypeId f32_type_id = get_builtin_type_id(TypeKind::F32);
        if (requested_type_id == TypeId::invalid()) {
            expression->target_type_id = f32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = f32_type_id};
        }
        const Type* requested_type = type_table.lookup(requested_type_id);
        if (is_floating_point_type(requested_type)) {
            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = f32_type_id};
        }
    }

    // clang-format off
    TypeCheckResult TypeCheckPass::typecheck_integer_literal_expression(IntegerLiteralExpression* expression,
        TypeId requested_type_id,
        bool is_negative) const
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->value_id != StringId::invalid());
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const TypeId i32_type_id = get_builtin_type_id(TypeKind::I32);
        if (requested_type_id == TypeId::invalid()) {
            expression->target_type_id = i32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = i32_type_id};
        }
        const Type* requested_type = type_table.lookup(requested_type_id);
        if (is_integer_type(requested_type)) {
            constexpr const uint8_t radix = 10;
            const std::string& literal_string = StringPool::get().lookup(expression->value_id);
            const uint32_t type_bitwidth = get_integer_bitwidth(requested_type);
            // Represents the bits needed for the *absolute* value of the literal
            const uint32_t needed_bitwidth = llvm::APInt::getBitsNeeded(literal_string, radix);
            if (needed_bitwidth > type_bitwidth) {
                const std::string message = std::format("Out of bounds integer literal: {}{} is too big for type '{}'",
                    is_negative ? "-" : "",
                    literal_string,
                    *requested_type);
                diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }

            if (is_signed_integer_type(requested_type) && needed_bitwidth == type_bitwidth) {
                // If a signed integer needs all bits, it has to be the minimum negative value
                if (!is_negative) {
                    const std::string message = std::format("Out of bounds integer literal: {} is too big for type '{}'", literal_string, *requested_type);
                    diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
                }

                const llvm::APInt absolute_llvm_value(type_bitwidth, literal_string, radix);
                if (absolute_llvm_value != llvm::APInt::getSignMask(type_bitwidth)) {
                    const std::string message = std::format("Out of bounds integer literal: -{} is too small for type '{}'",
                        literal_string,
                        *requested_type);
                    diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
                }
            } else if (is_unsigned_integer_type(requested_type) && is_negative) {
                const std::string message = std::format("Out of bounds integer literal: negative value -{} can't be assigned to unsigned type '{}'",
                    literal_string,
                    *requested_type);
                diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }

            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else if (is_floating_point_type(requested_type)) {
            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = i32_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const TypeId string_type_id = get_builtin_type_id(TypeKind::String);
        if (requested_type_id == string_type_id || requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = string_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = string_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_binary_expression(BinaryExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        const TypeId bool_type_id = get_builtin_type_id(TypeKind::Bool);
        if (requested_type_id == bool_type_id) {
            if (!is_boolean_operator(expression->operator_type)) {
                const std::string message = std::format("Type mismatch: Binary operator '{}' doesn't produce a boolean value", expression->operator_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
        }

        const TypeCheckResult lhs_tcr = typecheck_node(expression->lhs, TypeId::invalid());
        KPL_ASSERT_THAT(lhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (lhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(lhs_tcr.type_id != TypeId::invalid());

        const TypeCheckResult rhs_tcr = typecheck_node(expression->rhs, TypeId::invalid());
        KPL_ASSERT_THAT(rhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (rhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(rhs_tcr.type_id != TypeId::invalid());

        // Collect the candidate symbols
        const Type* lhs_type = type_table.lookup(lhs_tcr.type_id);
        const Symbol* lhs_type_symbol = symbol_table.lookup(lhs_type->symbol_id);
        KPL_ASSERT_THAT(lhs_type_symbol->get_symbol_kind() == SymbolKind::Type);
        const TypeSymbol* lhs_type_type_symbol = static_cast<const TypeSymbol*>(lhs_type_symbol);
        // Don't make this a reference because it could be invalidated by the StringPool::get().store() and we still need this identifier later
        const std::string operator_identifier = StringPool::get().lookup(get_operator_name_id(expression->operator_type));
        const std::string& lhs_type_identifier = StringPool::get().lookup(lhs_type->identifier_id);
        const StringId lhs_method_identifier_id = StringPool::get().store(lhs_type_identifier + '.' + operator_identifier);
        std::vector<SymbolId> candidate_symbol_ids;
        for (SymbolId method_symbol_id : lhs_type_type_symbol->method_symbol_ids) {
            const Symbol* method_symbol = symbol_table.lookup(method_symbol_id);
            if (method_symbol->identifier_id == lhs_method_identifier_id) {
                candidate_symbol_ids.push_back(method_symbol_id);
            }
        }
        if (lhs_tcr.type_id != rhs_tcr.type_id) {
            const Type* rhs_type = type_table.lookup(rhs_tcr.type_id);
            const Symbol* rhs_type_symbol = symbol_table.lookup(rhs_type->symbol_id);
            KPL_ASSERT_THAT(rhs_type_symbol->get_symbol_kind() == SymbolKind::Type);
            const TypeSymbol* rhs_type_type_symbol = static_cast<const TypeSymbol*>(rhs_type_symbol);
            const std::string& rhs_type_identifier = StringPool::get().lookup(rhs_type->identifier_id);
            const StringId rhs_method_identifier_id = StringPool::get().store(rhs_type_identifier + '.' + operator_identifier);
            for (SymbolId method_symbol_id : rhs_type_type_symbol->method_symbol_ids) {
                const Symbol* method_symbol = symbol_table.lookup(method_symbol_id);
                if (method_symbol->identifier_id == rhs_method_identifier_id) {
                    candidate_symbol_ids.push_back(method_symbol_id);
                }
            }
        }

        const std::vector<TypeId> argument_type_ids = {lhs_tcr.type_id, rhs_tcr.type_id};
        const std::vector<Expression*> arguments = {expression->lhs, expression->rhs};
        const auto method_symbol_id = find_matching_prototype_symbol(requested_type_id,
            candidate_symbol_ids,
            arguments,
            argument_type_ids);
        if (!method_symbol_id.has_value()) {
            const Diagnostic& diagnostic = method_symbol_id.error();
            if (diagnostic.code == DiagnosticCode::NoMatchingCallCandidates) {
                const Type* rhs_type = type_table.lookup(rhs_tcr.type_id);
                const std::string message = std::format("No implementation of binary operator '{}' between types '{}' and '{}'",
                    expression->operator_type,
                    *lhs_type,
                    *rhs_type);
                diagnostic_sink.report(DiagnosticCode::NoOperatorImplementation, std::move(message), expression->source_location);
            } else if (diagnostic.code == DiagnosticCode::MultipleMatchingCallCandidates) {
                const Type* rhs_type = type_table.lookup(rhs_tcr.type_id);
                const std::string message = std::format("Multiple implementations of binary operator '{}' between types '{}' and '{}' found:\n{}",
                    expression->operator_type,
                    *lhs_type,
                    *rhs_type,
                    std::move(diagnostic.message));
                diagnostic_sink.report(DiagnosticCode::MultipleOperatorImplementations, std::move(message), expression->source_location);
            } else {
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            }
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(method_symbol_id.value() != SymbolId::invalid());
        const Symbol* method_symbol = symbol_table.lookup(method_symbol_id.value());
        expression->target_type_id = method_symbol->type_id;
        expression->method_symbol_id = method_symbol_id.value();
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = method_symbol->type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_call_expression(CallExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id == SymbolId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        // Type check parameters first so the prototype candidates can be matched
        const TypeId void_type_id = get_builtin_type_id(TypeKind::Void);
        std::vector<TypeId> argument_type_ids;
        for (size_t i = 0; i < expression->arguments.size(); i++) {
            KPL_ASSERT_NOT_NULLPTR(expression->arguments[i]);
            const TypeCheckResult typecheck_result = typecheck_node(expression->arguments[i], TypeId::invalid());
            // Nodes cannot poison themselves without a diagnostic when the target type is up to them, hence the assert
            KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
            if (typecheck_result.type_id == void_type_id) {
                const Type* argument_type = type_table.lookup(typecheck_result.type_id);
                const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' cannot be of type void",
                    (i + 1),
                    get_full_identifier_from_path(expression->identifier_path),
                    *argument_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->arguments[i]->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            argument_type_ids.push_back(typecheck_result.type_id);
        }

        // Determine which of the candidates match the parameter types
        std::vector<SymbolId> candidate_symbol_ids;
        if (expression->cast_data.is_cast) {
            KPL_ASSERT_THAT(expression->candidate_symbol_ids.size() == 1);
            const Symbol* symbol = symbol_table.lookup(expression->candidate_symbol_ids[0]);
            KPL_ASSERT_THAT(symbol->get_symbol_kind() == SymbolKind::Type);
            const Type* target_type = type_table.lookup(symbol->type_id);

            // Check if it's a builtin cast to self and report a diagnostic
            if (is_builtin_type(target_type) && argument_type_ids.size() == 1 && target_type->id == argument_type_ids[0]) {
                const std::string message = std::format("Redundant cast of type '{}' to itself (the cast will be discarded, but consider removing it)",
                    *target_type);
                diagnostic_sink.report(DiagnosticCode::RedundantCast, std::move(message), expression->source_location);
                return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = target_type->id};
            }

            // No builtin cast to self -> collect candidate symbol ids
            const TypeSymbol* type_symbol = static_cast<const TypeSymbol*>(symbol);
            const StringId cast_identifier_id = StringPool::get().store(StringPool::get().lookup(type_symbol->identifier_id) + ".__cast");
            for (SymbolId method_symbol_id : type_symbol->method_symbol_ids) {
                KPL_ASSERT_THAT(method_symbol_id != SymbolId::invalid());
                const Symbol* method_symbol = symbol_table.lookup(method_symbol_id);
                if (method_symbol->identifier_id == cast_identifier_id) {
                    candidate_symbol_ids.push_back(method_symbol_id);
                }
            }
        } else {
            candidate_symbol_ids = expression->candidate_symbol_ids;
        }

        const auto matching_prototype_symbol_id = find_matching_prototype_symbol(requested_type_id,
            candidate_symbol_ids,
            expression->arguments,
            argument_type_ids);
        if (!matching_prototype_symbol_id.has_value()) {
            const Diagnostic& diagnostic = matching_prototype_symbol_id.error();
            if (diagnostic.code == DiagnosticCode::NoMatchingCallCandidates) {
                std::vector<Symbol*> candidate_symbols;
                candidate_symbols.reserve(expression->candidate_symbol_ids.size());
                for (SymbolId candidate_symbol_id : expression->candidate_symbol_ids) {
                    KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
                    candidate_symbols.push_back(symbol_table.lookup(candidate_symbol_id));
                }
                const StringId identifier_id = StringPool::get().store(get_full_identifier_from_path(expression->identifier_path));
                std::string message = std::format("No matching overload for function '{}'",
                    get_prototype_signature(identifier_id, argument_type_ids, false, type_table));
                const bool is_fully_qualified_call = !expression->module_path.identifier_parts.empty();
                if (is_fully_qualified_call) {
                    message += " in module '" + get_full_identifier_from_path(expression->module_path) + '\'';
                }
                message += " found";
                if (!candidate_symbols.empty()) {
                    message += ". Available candidates are:\n";
                    message += get_candidate_symbol_diagnostic_list(candidate_symbols, !is_fully_qualified_call, false, module_id, symbol_table, type_table);
                }
                diagnostic_sink.report(diagnostic.code, std::move(message), expression->source_location);
            } else if (diagnostic.code == DiagnosticCode::MultipleMatchingCallCandidates) {
                const bool is_fully_qualified_call = !expression->module_path.identifier_parts.empty();
                const StringId callee_signature_id = StringPool::get().store(get_full_identifier_from_path(expression->identifier_path));
                std::string message = std::format("Multiple matching overloads for function '{}'",
                    get_prototype_signature(callee_signature_id, argument_type_ids, false, type_table));
                if (is_fully_qualified_call) {
                    message += std::format(" in module '{}'", get_full_identifier_from_path(expression->module_path));
                }
                message += std::format(" found:\n{}", std::move(diagnostic.message));
                diagnostic_sink.report(diagnostic.code, std::move(message), expression->source_location);
            } else {
                diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            }
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        set_matching_call_candidate(expression, matching_prototype_symbol_id.value(), argument_type_ids);
        const Symbol* matching_prototype_symbol = symbol_table.lookup(matching_prototype_symbol_id.value());

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = matching_prototype_symbol->type_id};
    }

    // clang-format off
    std::expected<SymbolId, Diagnostic> TypeCheckPass::find_matching_prototype_symbol(TypeId return_type_id,
        const std::vector<SymbolId>& candidate_symbol_ids,
        const std::vector<Expression*>& arguments,
        const std::vector<TypeId>& argument_type_ids)
    {
        // clang-format on
        SymbolId candidate_symbol_id;
        const std::vector<MatchingCallCandidate> matching_call_candidates = find_matching_call_candidates(candidate_symbol_ids,
            argument_type_ids,
            arguments);
        if (matching_call_candidates.empty()) {
            // No matching candidates -> return the diagnostic code
            return std::unexpected(Diagnostic{.code = DiagnosticCode::NoMatchingCallCandidates, .message = ""});
        } else if (matching_call_candidates.size() == 1) {
            // Exactly one matching candidate -> return the candidate symbol id
            const MatchingCallCandidate& matching_call_candidate = matching_call_candidates[0];
            update_call_argument_target_types(matching_call_candidate.literal_type_ids_to_update, arguments);
            return matching_call_candidate.symbol_id;
        } else {
            // Multiple matching candidates -> check if there are exact matches
            std::map<MatchingCallCandidateKind, std::vector<MatchingCallCandidate>> sorted_matching_call_candidates;
            for (const MatchingCallCandidate& matching_call_candidate : matching_call_candidates) {
                sorted_matching_call_candidates[matching_call_candidate.match_kind].push_back(matching_call_candidate);
            }

            for (const auto& [match_kind, matching_call_candidate_group] : sorted_matching_call_candidates) {
                if (matching_call_candidate_group.empty()) {
                    // No matching candidates found for this match kind -> try the next match kind
                    continue;
                } else if (matching_call_candidate_group.size() == 1) {
                    // Exactly one matching candidate for this match kind found -> return the candidate symbol id
                    const MatchingCallCandidate& matching_call_candidate = matching_call_candidate_group[0];
                    update_call_argument_target_types(matching_call_candidate.literal_type_ids_to_update, arguments);
                    candidate_symbol_id = matching_call_candidate.symbol_id;
                    break;
                } else {
                    // Multiple matching candidates for this match kind (i. e. with the same priority) found -> return the diagnostic code
                    std::vector<Symbol*> matching_candidate_symbols;
                    matching_candidate_symbols.reserve(matching_call_candidate_group.size());
                    for (const MatchingCallCandidate& matching_call_candidate : matching_call_candidate_group) {
                        KPL_ASSERT_THAT(matching_call_candidate.symbol_id != SymbolId::invalid());
                        matching_candidate_symbols.push_back(symbol_table.lookup(matching_call_candidate.symbol_id));
                    }
                    return std::unexpected(Diagnostic{
                        .code = DiagnosticCode::MultipleMatchingCallCandidates,
                        .message = get_candidate_symbol_diagnostic_list(matching_candidate_symbols,
                            true,
                            false,
                            module_id,
                            symbol_table,
                            type_table),
                    });
                }
            }
        }

        // If we are here, there must be exactly one matching candidate symbol
        // No matching candidates as well as multiple matching candidates would have already made the function return
        KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
        const Symbol* candidate_symbol = symbol_table.lookup(candidate_symbol_id);
        KPL_ASSERT_THAT(candidate_symbol->type_id != TypeId::invalid());
        if (return_type_id != TypeId::invalid() && return_type_id != candidate_symbol->type_id) {
            const Type* target_type = type_table.lookup(candidate_symbol->type_id);
            const Type* requested_type = type_table.lookup(return_type_id);
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::TypeMismatch,
                .message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *target_type),
            });
        }
        return candidate_symbol_id;
    }

    // clang-format off
    std::vector<MatchingCallCandidate> TypeCheckPass::find_matching_call_candidates(const std::vector<SymbolId>& candidate_symbol_ids,
        const std::vector<TypeId>& argument_type_ids,
        const std::vector<Expression*>& arguments) const
    {
        // clang-format on
        KPL_ASSERT_THAT(arguments.size() == argument_type_ids.size());
        std::vector<MatchingCallCandidate> matching_call_candidates;
        for (SymbolId candidate_symbol_id : candidate_symbol_ids) {
            KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
            const Symbol* candidate_symbol = symbol_table.lookup(candidate_symbol_id);
            KPL_ASSERT_THAT(candidate_symbol->get_symbol_kind() == SymbolKind::Prototype);
            const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);

            // Determine how many arguments to typecheck and compare with the expected parameter types
            MatchingCallCandidate matching_call_candidate{.symbol_id = candidate_symbol_id, .match_kind = MatchingCallCandidateKind::ExactMatch};
            const bool is_variadic_candidate = candidate_prototype_symbol->is_variadic;
            const size_t parameter_count = candidate_prototype_symbol->parameter_type_ids.size();
            const size_t argument_count = argument_type_ids.size();
            size_t argument_count_to_typecheck = 0;
            if (parameter_count == argument_count) {
                argument_count_to_typecheck = argument_count;
                if (is_variadic_candidate) {
                    matching_call_candidate.match_kind = MatchingCallCandidateKind::VariadicMatch;
                }
            } else if (parameter_count < argument_count && is_variadic_candidate) {
                argument_count_to_typecheck = parameter_count;
                matching_call_candidate.match_kind = MatchingCallCandidateKind::VariadicMatch;
            } else {
                // This candidate cannot match because it has less parameters than arguments given and is not variadic
                // or it has more parameters than arguments given
                continue;
            }

            // Typecheck arguments and compare them with the expected parameter types
            for (size_t i = 0; i < argument_count_to_typecheck; i++) {
                KPL_ASSERT_NOT_NULLPTR(arguments[i]);
                const TypeId parameter_type_id = candidate_prototype_symbol->parameter_type_ids[i];
                KPL_ASSERT_THAT(parameter_type_id != TypeId::invalid());
                const TypeId argument_type_id = argument_type_ids[i];
                KPL_ASSERT_THAT(argument_type_id != TypeId::invalid());
                if (parameter_type_id == argument_type_id) {
                    continue;
                }

                // If the expected and actual types don't match we still check if the expression is a literal that can produce the expected type
                const Type* parameter_type = type_table.lookup(parameter_type_id);
                const Type* argument_type = type_table.lookup(argument_type_id);
                const ASTNodeType node_type = arguments[i]->node_type;
                bool argument_can_be_adjusted = false;
                // Integer argument and expected integer or floating point parameter
                // -> check if it'a a literal and store which type it should produce if this candidate is used
                if (is_integer_type(argument_type) && (is_integer_type(parameter_type) || is_floating_point_type(parameter_type))) {
                    if (node_type == ASTNodeType::IntegerLiteralExpression) {
                        matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type_id);
                        argument_can_be_adjusted = true;
                    } else if (node_type == ASTNodeType::MathematicalNegationExpression) {
                        const MathematicalNegationExpression* mathematical_negation_expression =
                            static_cast<const MathematicalNegationExpression*>(arguments[i]);
                        const ASTNodeType negated_expression_node_type = mathematical_negation_expression->expression->node_type;
                        if (negated_expression_node_type == ASTNodeType::IntegerLiteralExpression) {
                            matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type_id);
                            argument_can_be_adjusted = true;
                        }
                    }
                }
                // Floating point argument and expected floating point parameter
                // -> check if it'a a literal and store which type it should produce if this candidate is used
                else if (is_floating_point_type(argument_type) && is_floating_point_type(parameter_type)) {
                    if (node_type == ASTNodeType::IntegerLiteralExpression || node_type == ASTNodeType::FloatingPointLiteralExpression) {
                        matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type_id);
                        argument_can_be_adjusted = true;
                    } else if (node_type == ASTNodeType::MathematicalNegationExpression) {
                        const MathematicalNegationExpression* mathematical_negation_expression =
                            static_cast<const MathematicalNegationExpression*>(arguments[i]);
                        const ASTNodeType negated_expression_node_type = mathematical_negation_expression->expression->node_type;
                        if (negated_expression_node_type == ASTNodeType::IntegerLiteralExpression || negated_expression_node_type == ASTNodeType::FloatingPointLiteralExpression) {
                            matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type_id);
                            argument_can_be_adjusted = true;
                        }
                    }
                }

                // No literal at all or no literal that could produce the expected type -> candidate doesn't match
                if (!argument_can_be_adjusted) {
                    matching_call_candidate.symbol_id = SymbolId::invalid();
                    break;
                }

                // Update match kind
                if (matching_call_candidate.match_kind == MatchingCallCandidateKind::VariadicMatch) {
                    matching_call_candidate.match_kind = MatchingCallCandidateKind::VariadicMatchWithAdjustments;
                } else if (matching_call_candidate.match_kind == MatchingCallCandidateKind::ExactMatch) {
                    matching_call_candidate.match_kind = MatchingCallCandidateKind::ExactMatchWithAdjustments;
                }
            }
            if (matching_call_candidate.symbol_id != SymbolId::invalid()) {
                matching_call_candidates.push_back(std::move(matching_call_candidate));
            }
        }
        return matching_call_candidates;
    }

    // clang-format off
    void TypeCheckPass::update_call_argument_target_types(const std::unordered_map<uint32_t, TypeId>& argument_indices_to_update,
        const std::vector<Expression*>& arguments)
    {
        // clang-format on
        for (const auto [index, target_type_id] : argument_indices_to_update) {
            KPL_ASSERT_THAT(index < arguments.size());
            KPL_ASSERT_THAT(target_type_id != TypeId::invalid());
            Expression* argument = arguments[index];
            KPL_ASSERT_NOT_NULLPTR(argument);
            if (argument->node_type == ASTNodeType::IntegerLiteralExpression) {
                static_cast<IntegerLiteralExpression*>(argument)->target_type_id = target_type_id;
            } else if (argument->node_type == ASTNodeType::FloatingPointLiteralExpression) {
                static_cast<FloatingPointLiteralExpression*>(argument)->target_type_id = target_type_id;
            } else if (argument->node_type == ASTNodeType::MathematicalNegationExpression) {
                MathematicalNegationExpression* mathematical_negation_expression = static_cast<MathematicalNegationExpression*>(argument);
                const ASTNodeType negated_node_type = mathematical_negation_expression->expression->node_type;
                if (negated_node_type == ASTNodeType::IntegerLiteralExpression) {
                    static_cast<IntegerLiteralExpression*>(mathematical_negation_expression->expression)->target_type_id = target_type_id;
                } else if (negated_node_type == ASTNodeType::FloatingPointLiteralExpression) {
                    static_cast<FloatingPointLiteralExpression*>(mathematical_negation_expression->expression)->target_type_id = target_type_id;
                } else {
                    KPL_ASSERT_UNREACHABLE("Expected negated integer or floating point literal, received '{}'", negated_node_type);
                }
                mathematical_negation_expression->target_type_id = target_type_id;
            } else {
                KPL_ASSERT_UNREACHABLE("Expected integer or floating point literal or mathematical negation, received '{}'",
                    argument->node_type);
            }
        }
    }

    void TypeCheckPass::set_matching_call_candidate(CallExpression* expression, SymbolId candidate_symbol_id, const std::vector<TypeId>& argument_type_ids) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
        expression->symbol_id = candidate_symbol_id;

        // Check if it's a builtin cast
        // Criterias:
        // - Cast to a builtin type
        // - Has exactly one argument of a builtin type
        // - Is not variadic
        if (!expression->cast_data.is_cast) {
            return;
        }
        const Symbol* candidate_symbol = symbol_table.lookup(candidate_symbol_id);
        const Type* target_type = type_table.lookup(candidate_symbol->type_id);
        KPL_ASSERT_THAT(target_type->id != get_builtin_type_id(TypeKind::Void));
        if (!is_builtin_type(target_type)) {
            return;
        }

        if (argument_type_ids.size() != 1) {
            return;
        }
        const Type* original_argument_type = type_table.lookup(argument_type_ids[0]);
        if (!is_builtin_type(original_argument_type)) {
            return;
        }

        KPL_ASSERT_THAT(candidate_symbol->get_symbol_kind() == SymbolKind::Prototype);
        const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);
        if (candidate_prototype_symbol->is_variadic) {
            return;
        }

        // It's a builtin cast
        expression->cast_data.is_builtin_cast = true;
        expression->cast_data.original_argument_type_id = original_argument_type->id;
    }

    TypeCheckResult TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        TypeCheckResult typecheck_result;
        if (expression->expression->node_type == ASTNodeType::IntegerLiteralExpression) {
            typecheck_result = typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(expression->expression),
                requested_type_id,
                true);
        } else {
            typecheck_result = typecheck_node(expression->expression, requested_type_id);
        }
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        expression->target_type_id = typecheck_result.type_id;

        const Type* target_type = type_table.lookup(typecheck_result.type_id);
        const Symbol* target_type_symbol = symbol_table.lookup(target_type->symbol_id);
        KPL_ASSERT_THAT(target_type_symbol->get_symbol_kind() == SymbolKind::Type);
        const TypeSymbol* target_type_type_symbol = static_cast<const TypeSymbol*>(target_type_symbol);
        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        const auto method_symbol_id = target_type_type_symbol->find_method(StringPool::get().store("__mneg"),
            {typecheck_result.type_id},
            false,
            module_id,
            &symbol_table,
            &type_table);
        if (!method_symbol_id.has_value()) {
            const Diagnostic& diagnostic = method_symbol_id.error();
            diagnostic_sink.report(diagnostic.code, std::move(diagnostic.message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        if (method_symbol_id.value() == SymbolId::invalid()) {
            const std::string message = std::format("Type '{}' doesn't support unary operator '-'", *target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        expression->method_symbol_id = method_symbol_id.value();

        // If the data type supports mathematical negation but the typecheck didn't report a diagnostic we have to propagate that
        // because this ast node doesn't know a proper diagnostic
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = typecheck_result.type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = typecheck_result.type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_member_expression(MemberExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->member_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        if (requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = expression->member_type_id};
        }
        if (requested_type_id != expression->member_type_id) {
            const Type* requested_type = type_table.lookup(requested_type_id);
            const Type* member_type = type_table.lookup(expression->member_type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *member_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = expression->member_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_object_initializer_expression(ObjectInitializerExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        // expression->type_id can be TypeId::invalid(), which means that the type wasn't explicitely specified
        TypeId target_type_id;
        if (requested_type_id == TypeId::invalid()) {
            if (expression->type_id == TypeId::invalid()) {
                diagnostic_sink.report(DiagnosticCode::AmbiguousObjectInitializer,
                    "Can't clearly deduct which type the object initializer corresponds to. Specify the type explicitely in front of it (e. g. 'MyType{...}')",
                    expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            target_type_id = expression->type_id;
        } else {
            const Type* requested_type = type_table.lookup(requested_type_id);
            if (is_builtin_type(requested_type)) {
                const std::string message = std::format("Object initializers can only be used to create values for structs (expected '{}', which is not a struct)",
                    *requested_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }

            if (expression->type_id != TypeId::invalid() && requested_type_id != expression->type_id) {
                const Type* object_type = type_table.lookup(expression->type_id);
                const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *object_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->type_source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            target_type_id = requested_type_id;
        }

        expression->type_id = target_type_id;
        const Type* target_type = type_table.lookup(target_type_id);
        KPL_ASSERT_THAT(target_type->type_kind == TypeKind::Struct);
        const StructType* target_struct_type = static_cast<const StructType*>(target_type);
        for (size_t i = 0; i < expression->member_initializers.size(); i++) {
            const MemberInitializerData& member_initializer_data = expression->member_initializers[i];
            const StructTypeMember* struct_type_member = target_struct_type->find_member(member_initializer_data.identifier_id);
            if (struct_type_member == nullptr) {
                const std::string message = std::format("Member '{}' doesn't exist in type '{}'",
                    StringPool::get().lookup(member_initializer_data.identifier_id),
                    *target_type);
                diagnostic_sink.report(DiagnosticCode::UnknownMemberInInitializer, std::move(message), member_initializer_data.source_location);
                expression->node_type = ASTNodeType::Poison;
                continue;
            }
            KPL_ASSERT_THAT(struct_type_member->type_id != TypeId::invalid());
            const TypeCheckResult typecheck_result = typecheck_node(member_initializer_data.value_expression, struct_type_member->type_id);
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                continue;
            }
            KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                const Type* value_type = type_table.lookup(typecheck_result.type_id);
                const Type* member_type = type_table.lookup(struct_type_member->type_id);
                const std::string message = std::format("Type mismatch: Cannot assign value of type '{}' to member of type '{}'", *value_type, *member_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), member_initializer_data.value_expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                continue;
            }
        }

        if (expression->node_type == ASTNodeType::Poison) {
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = target_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_expression(VariableExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        const Symbol* symbol = symbol_table.lookup(expression->symbol_id);
        KPL_ASSERT_THAT(symbol->type_id != TypeId::invalid());
        if (requested_type_id == symbol->type_id || requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = symbol->type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = symbol->type_id};
        }
    }
}
