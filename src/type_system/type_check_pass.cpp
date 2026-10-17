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
#include "ast/expressions/cast_expression.hpp"
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
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/log.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <llvm/ADT/APInt.h>
#include <memory>
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

    TypeCheckResult TypeCheckPass::typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        TypeCheckResult::Status result_status = TypeCheckResult::Status::RequestFulfilled;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            KPL_ASSERT_NOT_NULLPTR(node);
            const TypeCheckResult typecheck_result = typecheck_node(node.get(), TypeId::invalid());
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
            case ASTNodeType::CastExpression:
                return typecheck_cast_expression(static_cast<CastExpression*>(node), requested_type_id);
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
                if constexpr (std::is_same_v<ValueType, std::unique_ptr<MemberExpression>>) {
                    KPL_ASSERT_THAT(expression->member_type_id != TypeId::invalid());
                    return expression->member_type_id;
                } else if constexpr (std::is_same_v<ValueType, std::unique_ptr<VariableExpression>>) {
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

        const TypeCheckResult typecheck_result = typecheck_node(statement->value_expression.get(), type_id);
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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
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

        const TypeCheckResult variable_tcr = typecheck_variable_definition_statement(statement->loop_variable_definition.get());
        KPL_ASSERT_THAT(variable_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (variable_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }
        KPL_ASSERT_THAT(variable_tcr.type_id != TypeId::invalid());

        const TypeCheckResult end_tcr = typecheck_node(statement->end_value.get(), variable_type_id);
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
            step_tcr = typecheck_node(statement->step_value.get(), variable_type_id);
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

        const TypeCheckResult condition_tcr = typecheck_node(statement->condition.get(), type_table.Builtins.bool_type_id);
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

        if (current_function_return_type_id == type_table.Builtins.void_type_id) {
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

        TypeCheckResult typecheck_result = typecheck_node(statement->expression.get(), current_function_return_type_id);
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
        if (statement->type_id == type_table.Builtins.void_type_id) {
            diagnostic_sink.report(DiagnosticCode::InvalidVariableType, "Cannot create a variable of type 'void'", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        const TypeCheckResult typecheck_result = typecheck_assignment_statement(statement->assignment_statement.get());
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
        if (requested_type_id == type_table.Builtins.bool_type_id || requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.bool_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = type_table.Builtins.bool_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        if (requested_type_id == TypeId::invalid()) {
            expression->target_type_id = type_table.Builtins.f32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.f32_type_id};
        }
        const Type* requested_type = type_table.lookup(requested_type_id);
        if (is_floating_point_type(requested_type)) {
            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = type_table.Builtins.f32_type_id};
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
        if (requested_type_id == TypeId::invalid()) {
            expression->target_type_id = type_table.Builtins.i32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.i32_type_id};
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
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = type_table.Builtins.i32_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        if (requested_type_id == type_table.Builtins.string_type_id || requested_type_id == TypeId::invalid()) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.string_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = type_table.Builtins.string_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_binary_expression(BinaryExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->lhs);
        KPL_ASSERT_NOT_NULLPTR(expression->rhs);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        if (requested_type_id == type_table.Builtins.bool_type_id) {
            if (!is_boolean_operator(expression->operator_type)) {
                const std::string message = std::format("Type mismatch: Binary operator '{}' doesn't produce a boolean value", expression->operator_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
        }

        TypeCheckResult lhs_tcr;
        TypeCheckResult rhs_tcr;
        if (requested_type_id == type_table.Builtins.bool_type_id || requested_type_id == TypeId::invalid()) {
            struct BinarySideTypecheckResult {
                ASTNode* expression = nullptr;
                std::string name;
            };

            // TODO (improvement): This could theoretically be done recursively for nested binary expressions to get better type deduction
            // Determine which expression should be typechecked first in order to get the correct type
            BinarySideTypecheckResult first_expression_to_typecheck;
            BinarySideTypecheckResult second_expression_to_typecheck;
            if (is_number_literal_expression(expression->lhs.get())) {
                if (is_number_literal_expression(expression->rhs.get())) {
                    // lhs and rhs are literals, so floats have to be typecheckd first
                    if (expression->lhs->node_type == ASTNodeType::FloatingPointLiteralExpression) {
                        // lhs is float, so typecheck lhs first
                        first_expression_to_typecheck = {.expression = expression->lhs.get(), .name = "lhs"};
                        second_expression_to_typecheck = {.expression = expression->rhs.get(), .name = "rhs"};
                    } else if (expression->rhs->node_type == ASTNodeType::FloatingPointLiteralExpression) {
                        // rhs is float, so typecheck rhs first
                        first_expression_to_typecheck = {.expression = expression->rhs.get(), .name = "rhs"};
                        second_expression_to_typecheck = {.expression = expression->lhs.get(), .name = "lhs"};
                    } else {
                        // Both are ints, order doesn't matter
                        first_expression_to_typecheck = {.expression = expression->lhs.get(), .name = "lhs"};
                        second_expression_to_typecheck = {.expression = expression->rhs.get(), .name = "rhs"};
                    }
                } else {
                    // lhs is literal, rhs is not, so typecheck rhs first
                    first_expression_to_typecheck = {.expression = expression->rhs.get(), .name = "rhs"};
                    second_expression_to_typecheck = {.expression = expression->lhs.get(), .name = "lhs"};
                }
            } else {
                // lhs is not a literal, so typecheck lhs first
                first_expression_to_typecheck = {.expression = expression->lhs.get(), .name = "lhs"};
                second_expression_to_typecheck = {.expression = expression->rhs.get(), .name = "rhs"};
            }

            const TypeCheckResult first_tcr = typecheck_node(first_expression_to_typecheck.expression, TypeId::invalid());
            KPL_ASSERT_THAT(first_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (first_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            KPL_ASSERT_THAT(first_tcr.type_id != TypeId::invalid());

            const ASTNodeType second_expression_node_type = second_expression_to_typecheck.expression->node_type;
            TypeCheckResult second_tcr = typecheck_node(second_expression_to_typecheck.expression, first_tcr.type_id);
            if (second_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            KPL_ASSERT_THAT(second_tcr.type_id != TypeId::invalid());
            if (second_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                // If the second expression can't procduce the same type as the first expression,
                // reset the node type and typecheck it again, but leave the target type up to the expression
                // That allows for operators that operate on different types
                second_expression_to_typecheck.expression->node_type = second_expression_node_type;
                second_tcr = typecheck_node(second_expression_to_typecheck.expression, TypeId::invalid());
                KPL_ASSERT_THAT(second_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
                if (second_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
                }
                KPL_ASSERT_THAT(second_tcr.type_id != TypeId::invalid());
            }

            if (first_expression_to_typecheck.expression == expression->lhs.get()) {
                lhs_tcr = first_tcr;
                rhs_tcr = second_tcr;
            } else {
                lhs_tcr = second_tcr;
                rhs_tcr = first_tcr;
            }
        } else {
            lhs_tcr = typecheck_binary_expression_side(expression, expression->lhs.get(), requested_type_id);
            KPL_ASSERT_THAT(lhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (lhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = lhs_tcr.type_id};
            }
            KPL_ASSERT_THAT(lhs_tcr.type_id != TypeId::invalid());

            rhs_tcr = typecheck_binary_expression_side(expression, expression->rhs.get(), requested_type_id);
            KPL_ASSERT_THAT(rhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (rhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = rhs_tcr.type_id};
            }
            KPL_ASSERT_THAT(rhs_tcr.type_id != TypeId::invalid());
        }

        const StringId operator_name_id = get_operator_name_id(expression->operator_type);
        const Type* lhs_type = type_table.lookup(lhs_tcr.type_id);
        if (!lhs_type->find_method(operator_name_id, {rhs_tcr.type_id})) {
            const Type* rhs_type = type_table.lookup(rhs_tcr.type_id);
            const std::string message = std::format("No implementation of binary operator '{}' between types '{}' and '{}'",
                expression->operator_type,
                *lhs_type,
                *rhs_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        // Use lhs_tcr.type instead of requested_type because that could be unknown
        expression->target_type_id = lhs_tcr.type_id;
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = lhs_tcr.type_id};
    }

    // clang-format off
    TypeCheckResult TypeCheckPass::typecheck_binary_expression_side(BinaryExpression* binary_expression,
        Expression* side_expression,
        TypeId requested_type_id)
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(binary_expression);
        KPL_ASSERT_THAT(binary_expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_NOT_NULLPTR(side_expression);
        KPL_ASSERT_THAT(side_expression->node_type != ASTNodeType::Poison);

        const TypeCheckResult typecheck_result = typecheck_node(side_expression, requested_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* requested_type = type_table.lookup(requested_type_id);
            const Type* expression_type = type_table.lookup(typecheck_result.type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *expression_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), side_expression->source_location);
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = typecheck_result.type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_call_expression(CallExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        // Type check parameters first so the prototype candidates can be matched
        std::vector<TypeId> argument_type_ids;
        for (size_t i = 0; i < expression->args.size(); i++) {
            KPL_ASSERT_NOT_NULLPTR(expression->args[i]);
            const TypeCheckResult typecheck_result = typecheck_node(expression->args[i].get(), TypeId::invalid());
            // Nodes cannot poison themselves without a diagnostic when the target type is up to them, hence the assert
            KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
            }
            KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
            if (typecheck_result.type_id == type_table.Builtins.void_type_id) {
                const Type* argument_type = type_table.lookup(typecheck_result.type_id);
                const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' cannot be of type void",
                    (i + 1),
                    get_full_identifier_from_path(expression->identifier_path),
                    *argument_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->args[i]->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.void_type_id};
            }
            argument_type_ids.push_back(typecheck_result.type_id);
        }

        // Determine which of the candidates match the parameter types
        std::vector<MatchingCallCandidate> matching_call_candidates = find_matching_call_candidates(expression, argument_type_ids);
        if (matching_call_candidates.empty()) {
            // No matching candidates -> print all available candidates
            const StringId identifier_id = StringPool::get().store(get_full_identifier_from_path(expression->identifier_path));
            std::string message = "No matching overload for function '";
            message += get_prototype_signature(identifier_id, argument_type_ids, false, type_table);
            message += '\'';
            if (!expression->module_path.identifier_parts.empty()) {
                message += " in module '" + get_full_identifier_from_path(expression->module_path) + '\'';
            }
            message += " found. Available candidates are:\n";
            for (size_t i = 0; i < expression->candidate_symbol_ids.size(); i++) {
                const SymbolId candidate_symbol_id = expression->candidate_symbol_ids[i];
                KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
                message += std::format("{}{}) {}", log::sub_line, i + 1, get_call_candidate_identifier_for_diagnostic(candidate_symbol_id));
                if (i < expression->candidate_symbol_ids.size() - 1) {
                    message += '\n';
                }
            }
            diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.void_type_id};
        } else if (matching_call_candidates.size() == 1) {
            update_call_argument_target_types(matching_call_candidates[0].literal_type_ids_to_update, expression->args);
            expression->symbol_id = matching_call_candidates[0].symbol_id;
        } else {
            // Multiple matching candidate -> check if there are exact matches
            std::vector<MatchingCallCandidate> exact_matching_call_candidate;
            for (MatchingCallCandidate& matching_call_candidate : matching_call_candidates) {
                if (matching_call_candidate.is_exact_match) {
                    exact_matching_call_candidate.push_back(std::move(matching_call_candidate));
                }
            }
            if (exact_matching_call_candidate.empty()) {
                // No exact matching candidates found -> multiple matching variadic functions found
                report_multiple_matching_call_candidates(expression, argument_type_ids, matching_call_candidates);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.void_type_id};
            } else if (exact_matching_call_candidate.size() == 1) {
                update_call_argument_target_types(exact_matching_call_candidate[0].literal_type_ids_to_update, expression->args);
                expression->symbol_id = exact_matching_call_candidate[0].symbol_id;
            } else {
                report_multiple_matching_call_candidates(expression, argument_type_ids, exact_matching_call_candidate);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.void_type_id};
            }
        }

        const Symbol* prototype_symbol = symbol_table.lookup(expression->symbol_id);
        KPL_ASSERT_THAT(prototype_symbol->type_id != TypeId::invalid());
        if (requested_type_id != prototype_symbol->type_id && requested_type_id != TypeId::invalid()) {
            const Type* target_type = type_table.lookup(prototype_symbol->type_id);
            const Type* requested_type = type_table.lookup(requested_type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *target_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = prototype_symbol->type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = prototype_symbol->type_id};
    }

    // clang-format off
    std::vector<MatchingCallCandidate> TypeCheckPass::find_matching_call_candidates(const CallExpression* expression,
        std::vector<TypeId> argument_type_ids) const
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->args.size() == argument_type_ids.size());
        std::vector<MatchingCallCandidate> matching_call_candidates;
        for (SymbolId candidate_symbol_id : expression->candidate_symbol_ids) {
            KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
            const Symbol* candidate_symbol = symbol_table.lookup(candidate_symbol_id);
            KPL_ASSERT_THAT(candidate_symbol->get_symbol_kind() == SymbolKind::Prototype);
            const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);

            // Determine how many arguments to typecheck and compare with the expected parameter types
            const bool is_variadic_candidate = candidate_prototype_symbol->is_variadic;
            const size_t parameter_count = candidate_prototype_symbol->parameter_type_ids.size();
            const size_t argument_count = argument_type_ids.size();
            MatchingCallCandidate matching_call_candidate{.symbol_id = candidate_symbol_id, .is_exact_match = !is_variadic_candidate};
            size_t argument_count_to_typecheck = 0;
            if (parameter_count == argument_count) {
                argument_count_to_typecheck = argument_count;
                if (candidate_prototype_symbol->is_variadic) {
                    matching_call_candidate.is_exact_match = false;
                }
            } else if (parameter_count < argument_count && candidate_prototype_symbol->is_variadic) {
                argument_count_to_typecheck = parameter_count;
                matching_call_candidate.is_exact_match = false;
            } else {
                continue;
            }

            // Typecheck arguments and compare them with the expected parameter types
            for (size_t i = 0; i < argument_count_to_typecheck; i++) {
                KPL_ASSERT_NOT_NULLPTR(expression->args[i]);
                KPL_ASSERT_THAT(candidate_prototype_symbol->parameter_type_ids[i] != TypeId::invalid());
                KPL_ASSERT_THAT(argument_type_ids[i] != TypeId::invalid());
                if (candidate_prototype_symbol->parameter_type_ids[i] == argument_type_ids[i]) {
                    continue;
                }

                // If the expected and actual types don't match we still check if the expression is a literal that can produce the expected type
                const Type* argument_type = type_table.lookup(argument_type_ids[i]);
                const Type* parameter_type = type_table.lookup(candidate_prototype_symbol->parameter_type_ids[i]);
                const ASTNodeType node_type = expression->args[i]->node_type;
                // Integer argument and expected integer or floating point parameter
                // -> check if it'a a literal and store which type it should produce if this candidate is used
                if (is_integer_type(argument_type) && (is_integer_type(parameter_type) || is_floating_point_type(parameter_type))) {
                    if (node_type == ASTNodeType::IntegerLiteralExpression) {
                        matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type->id);
                    } else if (node_type == ASTNodeType::MathematicalNegationExpression) {
                        const MathematicalNegationExpression* mathematical_negation_expression =
                            static_cast<const MathematicalNegationExpression*>(expression->args[i].get());
                        const ASTNodeType negated_expression_node_type = mathematical_negation_expression->expression->node_type;
                        if (negated_expression_node_type == ASTNodeType::IntegerLiteralExpression) {
                            matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type->id);
                        }
                    }
                }
                // Floating point argument and expected floating point parameter
                // -> check if it'a a literal and store which type it should produce if this candidate is used
                else if (is_floating_point_type(argument_type) && is_floating_point_type(parameter_type)) {
                    if (node_type == ASTNodeType::IntegerLiteralExpression || node_type == ASTNodeType::FloatingPointLiteralExpression) {
                        matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type->id);
                    } else if (node_type == ASTNodeType::MathematicalNegationExpression) {
                        const MathematicalNegationExpression* mathematical_negation_expression =
                            static_cast<const MathematicalNegationExpression*>(expression->args[i].get());
                        const ASTNodeType negated_expression_node_type = mathematical_negation_expression->expression->node_type;
                        if (negated_expression_node_type == ASTNodeType::IntegerLiteralExpression || negated_expression_node_type == ASTNodeType::FloatingPointLiteralExpression) {
                            matching_call_candidate.literal_type_ids_to_update.emplace(i, parameter_type->id);
                        }
                    }
                }
                // No literal that could produce the expected type -> Candidate doesn't match
                else {
                    matching_call_candidate.symbol_id = SymbolId::invalid();
                    break;
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
        const std::vector<std::unique_ptr<Expression>>& argument_expressions)
    {
        // clang-format on
        for (const auto [index, target_type_id] : argument_indices_to_update) {
            KPL_ASSERT_THAT(index < argument_expressions.size());
            KPL_ASSERT_THAT(target_type_id != TypeId::invalid());
            Expression* argument_expression = argument_expressions[index].get();
            KPL_ASSERT_NOT_NULLPTR(argument_expression);
            if (argument_expression->node_type == ASTNodeType::IntegerLiteralExpression) {
                static_cast<IntegerLiteralExpression*>(argument_expression)->target_type_id = target_type_id;
            } else if (argument_expression->node_type == ASTNodeType::FloatingPointLiteralExpression) {
                static_cast<FloatingPointLiteralExpression*>(argument_expression)->target_type_id = target_type_id;
            } else if (argument_expression->node_type == ASTNodeType::MathematicalNegationExpression) {
                MathematicalNegationExpression* mathematical_negation_expression = static_cast<MathematicalNegationExpression*>(argument_expression);
                const ASTNodeType negated_node_type = mathematical_negation_expression->expression->node_type;
                if (negated_node_type == ASTNodeType::IntegerLiteralExpression) {
                    static_cast<IntegerLiteralExpression*>(mathematical_negation_expression->expression.get())->target_type_id = target_type_id;
                } else if (negated_node_type == ASTNodeType::FloatingPointLiteralExpression) {
                    static_cast<FloatingPointLiteralExpression*>(mathematical_negation_expression->expression.get())->target_type_id = target_type_id;
                } else {
                    KPL_ASSERT_UNREACHABLE("Expected negated integer or floating point literal, received '{}'", negated_node_type);
                }
                mathematical_negation_expression->target_type_id = target_type_id;
            } else {
                KPL_ASSERT_UNREACHABLE("Expected integer or floating point literal or mathematical negation, received '{}'",
                    argument_expression->node_type);
            }
        }
    }

    // clang-format off
    void TypeCheckPass::report_multiple_matching_call_candidates(const CallExpression* expression,
        const std::vector<TypeId>& argument_type_ids,
        const std::vector<MatchingCallCandidate>& matching_call_candidates)
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(expression);
        const StringId identifier_id = StringPool::get().store(get_full_identifier_from_path(expression->identifier_path));
        std::string message = "Multiple matching overloads for function '";
        message += get_prototype_signature(identifier_id, argument_type_ids, false, type_table);
        message += '\'';
        if (!expression->module_path.identifier_parts.empty()) {
            message += " in module '" + get_full_identifier_from_path(expression->module_path) + '\'';
        }
        message += " found:\n";
        for (size_t i = 0; i < matching_call_candidates.size(); i++) {
            const SymbolId candidate_symbol_id = matching_call_candidates[i].symbol_id;
            KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
            message += std::format("{}{}) {}", log::sub_line, i + 1, get_call_candidate_identifier_for_diagnostic(candidate_symbol_id));
            if (i < matching_call_candidates.size() - 1) {
                message += '\n';
            }
        }
        diagnostic_sink.report(DiagnosticCode::UnknownSymbol, std::move(message), expression->source_location);
    }

    std::string TypeCheckPass::get_call_candidate_identifier_for_diagnostic(SymbolId candidate_symbol_id) const {
        KPL_ASSERT_THAT(candidate_symbol_id != SymbolId::invalid());
        const Symbol* candidate_symbol = symbol_table.lookup(candidate_symbol_id);
        KPL_ASSERT_THAT(candidate_symbol->get_symbol_kind() == SymbolKind::Prototype);
        const PrototypeSymbol* candidate_prototype_symbol = static_cast<const PrototypeSymbol*>(candidate_symbol);
        Module* candidate_module = symbol_table.lookup(candidate_symbol->module_id);
        const std::string& module_identifier = StringPool::get().lookup(candidate_module->full_identifier_id);
        const std::string candidate_signature = get_prototype_signature(candidate_prototype_symbol->identifier_id,
            candidate_prototype_symbol->parameter_type_ids,
            candidate_prototype_symbol->is_variadic,
            type_table);
        return std::format("{} (module {})", std::move(candidate_signature), module_identifier);
    }

    TypeCheckResult TypeCheckPass::typecheck_cast_expression(CastExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->original_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);

        if (expression->target_type_id == type_table.Builtins.void_type_id) {
            diagnostic_sink.report(DiagnosticCode::InvalidCast, "Cannot cast a value to type 'void'", expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        const Type* target_type = type_table.lookup(expression->target_type_id);
        if (requested_type_id != TypeId::invalid() && requested_type_id != expression->target_type_id) {
            const Type* requested_type = type_table.lookup(requested_type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        const TypeCheckResult typecheck_result = typecheck_node(expression->expression.get(), TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        expression->original_type_id = typecheck_result.type_id;

        if (expression->target_type_id == typecheck_result.type_id) {
            const std::string message = std::format("Redundant cast of type '{}' to itself (the cast will be discarded)", *target_type);
            diagnostic_sink.report(DiagnosticCode::RedundantCast, std::move(message), expression->source_location);
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = expression->target_type_id};
        }

        if (!target_type->find_method(StringPool::get().store("__cast"), {typecheck_result.type_id})) {
            const Type* original_type = type_table.lookup(typecheck_result.type_id);
            const std::string message = std::format("Type '{}' doesn't implement cast to type '{}'", *target_type, *original_type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = expression->target_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        TypeCheckResult typecheck_result;
        if (expression->expression->node_type == ASTNodeType::IntegerLiteralExpression) {
            typecheck_result = typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(expression->expression.get()),
                requested_type_id,
                true);
        } else {
            typecheck_result = typecheck_node(expression->expression.get(), requested_type_id);
        }
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        expression->target_type_id = typecheck_result.type_id;

        const Type* target_type = type_table.lookup(typecheck_result.type_id);
        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        if (!target_type->find_method(StringPool::get().store("__math_negate"), {typecheck_result.type_id})) {
            const std::string message = std::format("Type '{}' doesn't support unary operator '-'", *target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = TypeId::invalid()};
        }

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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = expression->member_type_id};
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
            if (expression->type_id != TypeId::invalid() && requested_type_id != expression->type_id) {
                const Type* requested_type = type_table.lookup(requested_type_id);
                const Type* object_type = type_table.lookup(expression->type_id);
                const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *object_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->type_source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = expression->type_id};
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
            const TypeCheckResult typecheck_result = typecheck_node(member_initializer_data.value_expression.get(), struct_type_member->type_id);
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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = target_type_id};
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
