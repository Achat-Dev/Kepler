// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_check_pass.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/function.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "lexer/operator_type.hpp"
#include "semantic_analysis/symbol.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    void TypeCheckPass::run() {
        typecheck_nodes(ast.top_level_nodes);
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

    TypeCheckResult TypeCheckPass::typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        TypeCheckResult::Status result_status = TypeCheckResult::Status::RequestFulfilled;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            const TypeCheckResult typecheck_result = typecheck_node(node.get(), type_table.Builtins.unknown_type);
            if (typecheck_result.is_poisoned()) {
                result_status = TypeCheckResult::Status::PoisonedWithDiagnostic;
            }
        }
        return {.status = result_status, .type = nullptr};
    }

    TypeCheckResult TypeCheckPass::typecheck_node(ASTNode* node, Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(node);
        KPL_ASSERT_NOT_NULLPTR(requested_type);

        switch (node->node_type) {
            case ASTNodeType::Poison:
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            case ASTNodeType::Extern:
                return {.status = TypeCheckResult::Status::RequestFulfilled, .type = nullptr};
            case ASTNodeType::Function:
                typecheck_function(static_cast<Function*>(node));
                return {.status = TypeCheckResult::Status::RequestFulfilled, .type = nullptr};
            case ASTNodeType::Prototype:
                KPL_ASSERT_UNREACHABLE("Cannot typecheck a prototype");
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
                return typecheck_boolean_literal_expression(static_cast<BooleanLiteralExpression*>(node), requested_type);
            case ASTNodeType::FloatingPointLiteralExpression:
                return typecheck_floating_point_literal_expression(static_cast<FloatingPointLiteralExpression*>(node), requested_type);
            case ASTNodeType::IntegerLiteralExpression:
                return typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(node), requested_type);
            case ASTNodeType::StringLiteralExpression:
                return typecheck_string_literal_expression(static_cast<StringLiteralExpression*>(node), requested_type);
            case ASTNodeType::BinaryExpression:
                return typecheck_binary_expression(static_cast<BinaryExpression*>(node), requested_type);
            case ASTNodeType::CallExpression:
                return typecheck_call_expression(static_cast<CallExpression*>(node), requested_type);
            case ASTNodeType::CastExpression:
                return typecheck_cast_expression(static_cast<CastExpression*>(node), requested_type);
            case ASTNodeType::MathematicalNegationExpression:
                return typecheck_mathematical_negation_expression(static_cast<MathematicalNegationExpression*>(node), requested_type);
            case ASTNodeType::VariableExpression:
                return typecheck_variable_expression(static_cast<VariableExpression*>(node), requested_type);
        }
        KPL_ASSERT_UNREACHABLE("Missing typecheck implementation for node type '{}'", node->node_type);
    }

    void TypeCheckPass::typecheck_function(Function* function) {
        KPL_ASSERT_NOT_NULLPTR(function);
        KPL_ASSERT_NOT_NULLPTR(function->prototype);
        KPL_ASSERT_NOT_NULLPTR(function->prototype->return_type);
        KPL_ASSERT_THAT(function->node_type != ASTNodeType::Poison, "Function must not be poisoned for type checking");
        KPL_ASSERT_THAT(current_function_return_type == nullptr, "Current function return type must be nullptr for type checking function");
        current_function_return_type = function->prototype->return_type;
        // Typecheck the body without poisoning the function so that the unpoisoned part of the body can still be accessed by later passes
        typecheck_nodes(function->body.nodes);
        current_function_return_type = nullptr;
    }

    TypeCheckResult TypeCheckPass::typecheck_assignment_statement(AssignmentStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "AssignmentStatement must not be poisoned for type checking");

        const Symbol* variable_symbol = statement->variable_expression->symbol;
        KPL_ASSERT_NOT_NULLPTR(variable_symbol);
        KPL_ASSERT_NOT_NULLPTR(variable_symbol->type);

        const TypeCheckResult typecheck_result = typecheck_node(statement->value_expression.get(), variable_symbol->type);
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: cannot assign a value of type '{}' to a variable of type '{}'",
                *typecheck_result.type,
                *variable_symbol->type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = variable_symbol->type};
    }

    TypeCheckResult TypeCheckPass::typecheck_for_statement(ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->loop_variable_definition);
        KPL_ASSERT_NOT_NULLPTR(statement->end_value);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ForStatement must not be posioned for type checking");

        Type* variable_type = statement->loop_variable_definition->type;
        KPL_ASSERT_NOT_NULLPTR(variable_type);
        if (!is_integer_type(variable_type)) {
            const std::string message = std::format("Loop variable of a for statement has to be an integer type, got '{}'", *variable_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->loop_variable_definition->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        const TypeCheckResult variable_tcr = typecheck_variable_definition_statement(statement->loop_variable_definition.get());
        KPL_ASSERT_NOT_NULLPTR(variable_tcr.type);
        if (variable_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }

        const TypeCheckResult end_tcr = typecheck_node(statement->end_value.get(), variable_type);
        KPL_ASSERT_NOT_NULLPTR(end_tcr.type);
        if (end_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        } else if (end_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *variable_type, *end_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->end_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        // Default value for the typecheck result of the step_value in case the step doesn't exist
        TypeCheckResult step_tcr{.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
        if (statement->step_value != nullptr) {
            step_tcr = typecheck_node(statement->step_value.get(), variable_type);
            KPL_ASSERT_NOT_NULLPTR(step_tcr.type);
        }
        if (step_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }
        if (step_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *variable_type, *step_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->step_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        const TypeCheckResult body_tcr = typecheck_nodes(statement->body.nodes);
        if (variable_tcr.is_poisoned() || end_tcr.is_poisoned() || step_tcr.is_poisoned() || body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_body_and_poison_for_statement(ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ForStatement must not be poisoned when poisoning it");
        statement->node_type = ASTNodeType::Poison;
        typecheck_nodes(statement->body.nodes);
        return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_if_statement(IfStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "IfStatement must not be poisoned for type checking");

        const TypeCheckResult condition_tcr = typecheck_node(statement->condition.get(), type_table.Builtins.bool_type);
        KPL_ASSERT_NOT_NULLPTR(condition_tcr.type);
        if (condition_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Condition of an if statement needs to be of type bool, got '{}'", *condition_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->condition->source_location);
        }

        const TypeCheckResult if_body_tcr = typecheck_nodes(statement->if_body.nodes);
        const TypeCheckResult else_body_tcr = typecheck_nodes(statement->else_body.nodes);
        if (condition_tcr.is_poisoned() || if_body_tcr.is_poisoned() || else_body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_return_statement(ReturnStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "ReturnStatement must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(current_function_return_type);
        KPL_ASSERT_THAT(current_function_return_type != type_table.Builtins.unknown_type, "Invalid function return type of unknown");

        if (current_function_return_type == type_table.Builtins.void_type) {
            if (statement->expression != nullptr) {
                diagnostic_sink.report(DiagnosticCode::InvalidReturn, "Cannot return a value from void function", statement->source_location);
                statement->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
        }

        if (statement->expression == nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn,
                "Expected expression after 'return' (non-void function needs a return value)",
                statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        TypeCheckResult typecheck_result = typecheck_node(statement->expression.get(), current_function_return_type);
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: cannot return a value of type '{}' from a function with a return type of '{}'",
                *typecheck_result.type,
                *current_function_return_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_definition_statement(VariableDefinitionStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison, "VariableDefinitionStatement must not be poisoned for type checking");

        KPL_ASSERT_NOT_NULLPTR(statement->type);
        if (statement->type == type_table.Builtins.void_type) {
            diagnostic_sink.report(DiagnosticCode::InvalidVariableType, "Cannot create a variable of type 'void'", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        const TypeCheckResult typecheck_result = typecheck_assignment_statement(statement->assignment_statement.get());
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
            "The AssignmentStatement of a VariableDefinitionStatement must not poison itself without a diagnostic during type checking");
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = statement->type};
    }

    TypeCheckResult TypeCheckPass::typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, const Type* requested_type) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BooleanLiteralExpression must be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        if (requested_type == type_table.Builtins.bool_type || requested_type == type_table.Builtins.unknown_type) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.bool_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.bool_type};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, Type* requested_type) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        KPL_ASSERT_THAT(expression->target_type == nullptr, "Target type of FloatingPointLiteralExpression must be nullptr for type checking");
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "FloatingPointLiteralExpression must not be poisoned for type checking");
        if (is_floating_point_type(requested_type)) {
            expression->target_type = requested_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = requested_type};
        } else if (requested_type == type_table.Builtins.unknown_type) {
            expression->target_type = type_table.Builtins.f32_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.f32_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.unknown_type};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_integer_literal_expression(IntegerLiteralExpression* expression, Type* requested_type) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->target_type == nullptr, "Target type of IntegerLiteralExpression must be nullptr for type checking");
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "IntegerLiteralExpression must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        if (is_integer_type(requested_type) || is_floating_point_type(requested_type)) {
            expression->target_type = requested_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = requested_type};
        } else if (requested_type == type_table.Builtins.unknown_type) {
            expression->target_type = type_table.Builtins.i32_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.i32_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.unknown_type};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, const Type* requested_type) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "StringLiteralExpression must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        if (requested_type == type_table.Builtins.string_type || requested_type == type_table.Builtins.unknown_type) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = type_table.Builtins.string_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.string_type};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_binary_expression(BinaryExpression* expression, Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->target_type == nullptr, "Target type of BinaryExpression must be nullptr for type checking");
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "BinaryExpression must node be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);

        TypeCheckResult lhs_tcr;
        TypeCheckResult rhs_tcr;
        if (requested_type == type_table.Builtins.bool_type) {
            if (!is_boolean_operator(expression->operator_type)) {
                const std::string message = std::format("Type mismatch: Binary operator '{}' doesn't produce a boolean value", expression->operator_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }

            // Pass unknown as the requested type so that the expressions don't try to produce a boolean individually
            lhs_tcr = typecheck_node(expression->lhs.get(), type_table.Builtins.unknown_type);
            KPL_ASSERT_NOT_NULLPTR(lhs_tcr.type);
            KPL_ASSERT_THAT(lhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
                "The lhs expression of a BinaryExpression must not poison itself without a diagnostic for type checking");
            if (lhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }

            // If the lhs is an integer or a floating point type and the rhs is an according literal, the expression should generate a specific type
            // Otherwise, it's up to the expression to determine it's type
            // This system is probably going to break later on as more features are added to language, but works fine for now
            // Also, need to use dynamic_cast here, otherwise it doesn't work correctly
            if (is_integer_type(lhs_tcr.type) && dynamic_cast<IntegerLiteralExpression*>(expression->rhs.get())) {
                rhs_tcr = typecheck_node(expression->rhs.get(), lhs_tcr.type);
            }
            // clang-format off
            else if (is_floating_point_type(lhs_tcr.type)
                && (dynamic_cast<IntegerLiteralExpression*>(expression->rhs.get())
                    || dynamic_cast<FloatingPointLiteralExpression*>(expression->rhs.get()))) {
                // clang-format on
                rhs_tcr = typecheck_node(expression->rhs.get(), lhs_tcr.type);
            } else {
                rhs_tcr = typecheck_node(expression->rhs.get(), type_table.Builtins.unknown_type);
            }

            KPL_ASSERT_NOT_NULLPTR(rhs_tcr.type);
            KPL_ASSERT_THAT(rhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
                "The rhs expression of a BinaryExpression must not poison itself without a diagnostic for type checking");
            if (rhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }
        } else {
            lhs_tcr = typecheck_binary_expression_side(expression, expression->lhs.get(), requested_type);
            KPL_ASSERT_NOT_NULLPTR(lhs_tcr.type);
            KPL_ASSERT_THAT(lhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
                "The lhs expression of a BinaryExpression must not poison itself without a diagnostic for type checking");
            if (lhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                return lhs_tcr;
            }

            rhs_tcr = typecheck_binary_expression_side(expression, expression->rhs.get(), requested_type);
            KPL_ASSERT_NOT_NULLPTR(rhs_tcr.type);
            KPL_ASSERT_THAT(rhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
                "The rhs expression of a BinaryExpression must not poison itself without a diagnostic for type checking");
            if (rhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                return rhs_tcr;
            }
        }

        const StringId operator_name_id = get_operator_name_id(expression->operator_type);
        if (!lhs_tcr.type->find_method(operator_name_id, {rhs_tcr.type})) {
            const std::string message = std::format("No implemention of binary operator '{}' between types '{}' and '{}'",
                expression->operator_type,
                *lhs_tcr.type,
                *rhs_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        // Use lhs_tcr.type instead of requested_type because that could be unknown
        expression->target_type = lhs_tcr.type;
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = lhs_tcr.type};
    }

    TypeCheckResult TypeCheckPass::typecheck_binary_expression_side(BinaryExpression* binary_expression, Expression* side_expression, Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(binary_expression);
        KPL_ASSERT_THAT(binary_expression->node_type != ASTNodeType::Poison, "BinaryExpression must not be poisoned for type checking side expression");
        KPL_ASSERT_NOT_NULLPTR(side_expression);
        KPL_ASSERT_THAT(side_expression->node_type != ASTNodeType::Poison,
            "Side expression of BinaryExpression must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);

        const TypeCheckResult typecheck_result = typecheck_node(side_expression, requested_type);
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *typecheck_result.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), side_expression->source_location);
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = requested_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_call_expression(CallExpression* expression, const Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CallExpression must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        const Symbol* prototype_symbol = expression->symbol;
        KPL_ASSERT_NOT_NULLPTR(prototype_symbol);
        if (requested_type != prototype_symbol->type && requested_type != type_table.Builtins.unknown_type) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = prototype_symbol->type};
        }

        KPL_ASSERT_HOLDS_ALTERNATIVE(prototype_symbol->data, PrototypeSymbolData, "Prototype symbol of CallExpression");
        const std::vector<Type*>& parameter_types = std::get<PrototypeSymbolData>(prototype_symbol->data).parameter_types;
        KPL_ASSERT_THAT(parameter_types.size() == expression->args.size(), "Mismatching call parameter count during typechecking");

        for (size_t i = 0; i < parameter_types.size(); i++) {
            KPL_ASSERT_NOT_NULLPTR(expression->args[i]);
            KPL_ASSERT_NOT_NULLPTR(parameter_types[i]);
            const TypeCheckResult typecheck_result = typecheck_node(expression->args[i].get(), parameter_types[i]);
            KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = prototype_symbol->type};
            } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                KPL_ASSERT_THAT(typecheck_result.type != parameter_types[i], "Poisoned because of mismatching call parameter type without diagnostic");
                const std::string_view identifier = StringPool::get().lookup(prototype_symbol->identifier_id);
                const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' expects type '{}', but the given argument is of type '{}'",
                    (i + 1),
                    identifier,
                    *parameter_types[i],
                    *typecheck_result.type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, expression->args[i]->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = prototype_symbol->type};
            }
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = prototype_symbol->type};
    }

    TypeCheckResult TypeCheckPass::typecheck_cast_expression(CastExpression* expression, const Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_NOT_NULLPTR(expression->target_type);
        KPL_ASSERT_THAT(expression->original_type == nullptr, "Original type of CastExpression must be nullptr for type checking");
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "CastExpression must not be posioned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);

        if (expression->target_type == type_table.Builtins.void_type) {
            diagnostic_sink.report(DiagnosticCode::InvalidCast, "Cannot cast a value to type 'void'", expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        if (requested_type != type_table.Builtins.unknown_type && requested_type != expression->target_type) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *expression->target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        const TypeCheckResult typecheck_result = typecheck_node(expression->expression.get(), type_table.Builtins.unknown_type);
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic,
            "The expression of a CastExpression must not poison itself without a diagnostic for type checking");
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        expression->original_type = typecheck_result.type;

        if (!expression->target_type->find_method(StringPool::get().store("__cast"), {typecheck_result.type})) {
            const std::string message = std::format("Type '{}' doesn't implement cast to type '{}'", *expression->target_type, *typecheck_result.type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        if (expression->target_type == typecheck_result.type) {
            const std::string message = std::format("Redundant cast of type '{}' to itself (the cast will be discarded)", *typecheck_result.type);
            diagnostic_sink.report(DiagnosticCode::RedundantCast, std::move(message), expression->source_location);
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = expression->target_type};
    }

    TypeCheckResult TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, Type* requested_type) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type == nullptr, "Target type of MathematicalNegationExpression must be nullptr for type checking");
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "MathematicalNegationExpression must not be poisoned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);

        TypeCheckResult typecheck_result = typecheck_node(expression->expression.get(), requested_type);
        KPL_ASSERT_NOT_NULLPTR(typecheck_result.type);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }
        expression->target_type = typecheck_result.type;

        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        if (!typecheck_result.type->find_method(StringPool::get().store("__math_negate"), {typecheck_result.type})) {
            const std::string message = std::format("Type '{}' doesn't support unary operator '-'", *typecheck_result.type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }

        // If the data type supports mathematical negation but the typecheck didn't report a diagnostic we have to propagate that
        // because this ast node doesn't know a proper diagnostic
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = typecheck_result.type};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type = typecheck_result.type};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_expression(VariableExpression* expression, const Type* requested_type) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->symbol);
        KPL_ASSERT_NOT_NULLPTR(expression->symbol->type);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison, "VariableExpression must not be posioned for type checking");
        KPL_ASSERT_NOT_NULLPTR(requested_type);
        if (requested_type == expression->symbol->type || requested_type == type_table.Builtins.unknown_type) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type = expression->symbol->type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type = expression->symbol->type};
        }
    }
}
