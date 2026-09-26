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
#include <cstdint>
#include <format>
#include <llvm/ADT/APInt.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    // TODO (check): Check the usage of TypeId::invalid() and unknown_type_id for resulting type id
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
            const TypeCheckResult typecheck_result = typecheck_node(node.get(), type_table.Builtins.unknown_type_id);
            if (typecheck_result.is_poisoned()) {
                result_status = TypeCheckResult::Status::PoisonedWithDiagnostic;
            }
        }
        return {.status = result_status, .type_id = TypeId::invalid()};
    }

    TypeCheckResult TypeCheckPass::typecheck_node(ASTNode* node, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(node);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        switch (node->node_type) {
            case ASTNodeType::Struct:
            case ASTNodeType::Prototype:
            case ASTNodeType::ImportStatement:
            case ASTNodeType::ModuleStatement:
                KPL_ASSERT_UNREACHABLE("Cannot typecheck a node of type '{}' as part of the top level nodes", node->node_type);

            case ASTNodeType::Poison:
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
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
        KPL_ASSERT_NOT_NULLPTR(statement->variable_expression);
        KPL_ASSERT_THAT(statement->variable_expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_NOT_NULLPTR(statement->value_expression);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        const Symbol* variable_symbol = symbol_table.lookup(statement->variable_expression->symbol_id);
        KPL_ASSERT_THAT(variable_symbol->type_id != TypeId::invalid());

        const TypeCheckResult typecheck_result = typecheck_node(statement->value_expression.get(), variable_symbol->type_id);
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* value_type = type_table.lookup(typecheck_result.type_id);
            const Type* variable_type = type_table.lookup(variable_symbol->type_id);
            const std::string message = std::format("Type mismatch: cannot assign a value of type '{}' to a variable of type '{}'",
                *value_type,
                *variable_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = variable_symbol->type_id};
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
        KPL_ASSERT_THAT(variable_tcr.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(variable_tcr.type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(variable_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (variable_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }

        const TypeCheckResult end_tcr = typecheck_node(statement->end_value.get(), variable_type_id);
        KPL_ASSERT_THAT(end_tcr.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(end_tcr.type_id != type_table.Builtins.unknown_type_id);
        if (end_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        } else if (end_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* end_value_type = type_table.lookup(end_tcr.type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *variable_type, *end_value_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->end_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        // Default value for the typecheck result of the step_value in case the step doesn't exist
        TypeCheckResult step_tcr{.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.unknown_type_id};
        if (statement->step_value != nullptr) {
            step_tcr = typecheck_node(statement->step_value.get(), variable_type_id);
            KPL_ASSERT_THAT(step_tcr.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(step_tcr.type_id != type_table.Builtins.unknown_type_id);
            if (step_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                return typecheck_body_and_poison_for_statement(statement);
            }
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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.unknown_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_body_and_poison_for_statement(ForStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        statement->node_type = ASTNodeType::Poison;
        typecheck_nodes(statement->body.nodes);
        return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_if_statement(IfStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->condition);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        const TypeCheckResult condition_tcr = typecheck_node(statement->condition.get(), type_table.Builtins.bool_type_id);
        KPL_ASSERT_THAT(condition_tcr.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(condition_tcr.type_id != type_table.Builtins.unknown_type_id);
        if (condition_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* condition_type = type_table.lookup(condition_tcr.type_id);
            const std::string message = std::format("Condition of an IfStatement needs to be of type bool, got '{}'", *condition_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->condition->source_location);
        }

        const TypeCheckResult if_body_tcr = typecheck_nodes(statement->if_body.nodes);
        const TypeCheckResult else_body_tcr = typecheck_nodes(statement->else_body.nodes);
        if (condition_tcr.is_poisoned() || if_body_tcr.is_poisoned() || else_body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.unknown_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_return_statement(ReturnStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(current_function_return_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(current_function_return_type_id != type_table.Builtins.unknown_type_id);

        if (current_function_return_type_id == type_table.Builtins.void_type_id) {
            if (statement->expression != nullptr) {
                diagnostic_sink.report(DiagnosticCode::InvalidReturn, "Cannot return a value from void function", statement->source_location);
                statement->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.unknown_type_id};
        }

        if (statement->expression == nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn,
                "Expected expression after 'return' (non-void function needs a return value)",
                statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }

        TypeCheckResult typecheck_result = typecheck_node(statement->expression.get(), current_function_return_type_id);
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* expression_type = type_table.lookup(typecheck_result.type_id);
            const Type* function_return_type = type_table.lookup(current_function_return_type_id);
            const std::string message = std::format("Type mismatch: cannot return a value of type '{}' from a function with a return type of '{}'",
                *expression_type,
                *function_return_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.unknown_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_definition_statement(VariableDefinitionStatement* statement) {
        KPL_ASSERT_NOT_NULLPTR(statement);
        KPL_ASSERT_NOT_NULLPTR(statement->assignment_statement);
        KPL_ASSERT_THAT(statement->node_type != ASTNodeType::Poison);

        KPL_ASSERT_THAT(statement->type_id != TypeId::invalid());
        if (statement->type_id == type_table.Builtins.void_type_id) {
            diagnostic_sink.report(DiagnosticCode::InvalidVariableType, "Cannot create a variable of type 'void'", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }

        const TypeCheckResult typecheck_result = typecheck_assignment_statement(statement->assignment_statement.get());
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = statement->type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        if (requested_type_id == type_table.Builtins.bool_type_id || requested_type_id == type_table.Builtins.unknown_type_id) {
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
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        const Type* requested_type = type_table.lookup(requested_type_id);
        if (is_floating_point_type(requested_type)) {
            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else if (requested_type_id == type_table.Builtins.unknown_type_id) {
            expression->target_type_id = type_table.Builtins.f32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.f32_type_id};
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
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        constexpr const uint8_t radix = 10;
        const Type* requested_type = type_table.lookup(requested_type_id);
        if (is_integer_type(requested_type)) {
            const std::string_view literal_string = StringPool::get().lookup(expression->value_id);
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
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }

            if (is_signed_integer_type(requested_type) && needed_bitwidth == type_bitwidth) {
                // If a signed integer needs all bits, it has to be the minimum negative value
                if (!is_negative) {
                    const std::string message = std::format("Out of bounds integer literal: {} is too big for type '{}'", literal_string, *requested_type);
                    diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
                }

                const llvm::APInt absolute_llvm_value(type_bitwidth, literal_string, radix);
                if (absolute_llvm_value != llvm::APInt::getSignMask(type_bitwidth)) {
                    const std::string message = std::format("Out of bounds integer literal: -{} is too small for type '{}'",
                        literal_string,
                        *requested_type);
                    diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
                }
            } else if (is_unsigned_integer_type(requested_type) && is_negative) {
                const std::string message = std::format("Out of bounds integer literal: negative value -{} can't be assigned to unsigned type '{}'",
                    literal_string,
                    *requested_type);
                diagnostic_sink.report(DiagnosticCode::IntegerLiteralOutOfBounds, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }

            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else if (is_floating_point_type(requested_type)) {
            expression->target_type_id = requested_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = requested_type_id};
        } else if (requested_type_id == type_table.Builtins.unknown_type_id) {
            expression->target_type_id = type_table.Builtins.i32_type_id;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = type_table.Builtins.i32_type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = type_table.Builtins.i32_type_id};
        }
    }

    TypeCheckResult TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        if (requested_type_id == type_table.Builtins.string_type_id || requested_type_id == type_table.Builtins.unknown_type_id) {
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
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());

        if (requested_type_id == type_table.Builtins.bool_type_id) {
            if (!is_boolean_operator(expression->operator_type)) {
                const std::string message = std::format("Type mismatch: Binary operator '{}' doesn't produce a boolean value", expression->operator_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }
        }

        TypeCheckResult lhs_tcr;
        TypeCheckResult rhs_tcr;
        if (requested_type_id == type_table.Builtins.bool_type_id || requested_type_id == type_table.Builtins.unknown_type_id) {
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

            const TypeCheckResult first_tcr = typecheck_node(first_expression_to_typecheck.expression, type_table.Builtins.unknown_type_id);
            KPL_ASSERT_THAT(first_tcr.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(first_tcr.type_id != type_table.Builtins.unknown_type_id);
            KPL_ASSERT_THAT(first_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (first_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }

            const ASTNodeType second_expression_node_type = second_expression_to_typecheck.expression->node_type;
            TypeCheckResult second_tcr = typecheck_node(second_expression_to_typecheck.expression, first_tcr.type_id);
            KPL_ASSERT_THAT(second_tcr.type_id != TypeId::invalid());
            if (second_tcr.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                // If the second expression can't procduce the same type as the first expression,
                // reset the node type and typecheck it again, but leave the target type up to the expression
                // That allows for operators that operate on different types
                second_expression_to_typecheck.expression->node_type = second_expression_node_type;
                second_tcr = typecheck_node(second_expression_to_typecheck.expression, type_table.Builtins.unknown_type_id);
                KPL_ASSERT_THAT(second_tcr.type_id != TypeId::invalid());
                KPL_ASSERT_THAT(second_tcr.type_id != type_table.Builtins.unknown_type_id);
                KPL_ASSERT_THAT(second_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
                if (second_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
                }
            } else if (second_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
            }
            KPL_ASSERT_THAT(second_tcr.type_id != type_table.Builtins.unknown_type_id);

            if (first_expression_to_typecheck.expression == expression->lhs.get()) {
                lhs_tcr = first_tcr;
                rhs_tcr = second_tcr;
            } else {
                lhs_tcr = second_tcr;
                rhs_tcr = first_tcr;
            }
        } else {
            lhs_tcr = typecheck_binary_expression_side(expression, expression->lhs.get(), requested_type_id);
            KPL_ASSERT_THAT(lhs_tcr.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(lhs_tcr.type_id != type_table.Builtins.unknown_type_id);
            KPL_ASSERT_THAT(lhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (lhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = lhs_tcr.type_id};
            }

            rhs_tcr = typecheck_binary_expression_side(expression, expression->rhs.get(), requested_type_id);
            KPL_ASSERT_THAT(rhs_tcr.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(rhs_tcr.type_id != type_table.Builtins.unknown_type_id);
            KPL_ASSERT_THAT(rhs_tcr.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
            if (rhs_tcr.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = rhs_tcr.type_id};
            }
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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
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
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());

        const TypeCheckResult typecheck_result = typecheck_node(side_expression, requested_type_id);
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            const Type* requested_type = type_table.lookup(requested_type_id);
            const Type* expression_type = type_table.lookup(typecheck_result.type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *expression_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), side_expression->source_location);
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = typecheck_result.type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_call_expression(CallExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        const Symbol* prototype_symbol = symbol_table.lookup(expression->symbol_id);
        if (requested_type_id != prototype_symbol->type_id && requested_type_id != type_table.Builtins.unknown_type_id) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = prototype_symbol->type_id};
        }

        KPL_ASSERT_THAT(std::holds_alternative<PrototypeSymbolData>(prototype_symbol->data));
        const PrototypeSymbolData prototype_symbol_data = std::get<PrototypeSymbolData>(prototype_symbol->data);
        const std::vector<TypeId>& parameter_type_ids = prototype_symbol_data.parameter_type_ids;
        if (!prototype_symbol_data.is_variadic) {
            KPL_ASSERT_THAT(parameter_type_ids.size() == expression->args.size());
        } else {
            KPL_ASSERT_THAT(parameter_type_ids.size() <= expression->args.size());
        }

        for (size_t i = 0; i < parameter_type_ids.size(); i++) {
            KPL_ASSERT_NOT_NULLPTR(expression->args[i]);
            KPL_ASSERT_THAT(parameter_type_ids[i] != TypeId::invalid());
            const TypeCheckResult typecheck_result = typecheck_node(expression->args[i].get(), parameter_type_ids[i]);
            KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
            KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
            if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = prototype_symbol->type_id};
            } else if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
                KPL_ASSERT_THAT(typecheck_result.type_id != parameter_type_ids[i]);
                const Type* parameter_type = type_table.lookup(parameter_type_ids[i]);
                const Type* argument_type = type_table.lookup(typecheck_result.type_id);
                const std::string_view identifier = StringPool::get().lookup(prototype_symbol->identifier_id);
                const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' expects type '{}', but the given argument is of type '{}'",
                    (i + 1),
                    identifier,
                    *parameter_type,
                    *argument_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, expression->args[i]->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = prototype_symbol->type_id};
            }
        }

        // Check the variadic arguments
        if (prototype_symbol_data.is_variadic) {
            // Since variadic arguments don't have a specific type to produce, just make sure that the aren't producing 'void'
            for (size_t i = parameter_type_ids.size(); i < expression->args.size(); i++) {
                KPL_ASSERT_NOT_NULLPTR(expression->args[i]);
                const TypeCheckResult typecheck_result = typecheck_node(expression->args[i].get(), type_table.Builtins.unknown_type_id);
                KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
                KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
                KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
                if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = prototype_symbol->type_id};
                }
                if (typecheck_result.type_id == type_table.Builtins.void_type_id) {
                    diagnostic_sink.report(DiagnosticCode::TypeMismatch,
                        "Type mismatch: A variadic argument can't be of type 'void'",
                        expression->args[i]->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = prototype_symbol->type_id};
                }
            }
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = prototype_symbol->type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_cast_expression(CastExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id != TypeId::invalid());
        KPL_ASSERT_THAT(expression->original_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());

        if (expression->target_type_id == type_table.Builtins.void_type_id) {
            diagnostic_sink.report(DiagnosticCode::InvalidCast, "Cannot cast a value to type 'void'", expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }

        const Type* target_type = type_table.lookup(expression->target_type_id);
        if (requested_type_id != type_table.Builtins.unknown_type_id && requested_type_id != expression->target_type_id) {
            const Type* requested_type = type_table.lookup(requested_type_id);
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", *requested_type, *target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }

        const TypeCheckResult typecheck_result = typecheck_node(expression->expression.get(), type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        KPL_ASSERT_THAT(typecheck_result.status != TypeCheckResult::Status::PoisonedWithoutDiagnostic);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }
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
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = type_table.Builtins.unknown_type_id};
        }

        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = expression->target_type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, TypeId requested_type_id) {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_NOT_NULLPTR(expression->expression);
        KPL_ASSERT_THAT(expression->target_type_id == TypeId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        TypeCheckResult typecheck_result;
        if (expression->expression->node_type == ASTNodeType::IntegerLiteralExpression) {
            typecheck_result = typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(expression->expression.get()),
                requested_type_id,
                true);
        } else {
            typecheck_result = typecheck_node(expression->expression.get(), requested_type_id);
        }
        KPL_ASSERT_THAT(typecheck_result.type_id != TypeId::invalid());
        KPL_ASSERT_THAT(typecheck_result.type_id != type_table.Builtins.unknown_type_id);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
        }
        expression->target_type_id = typecheck_result.type_id;

        const Type* target_type = type_table.lookup(typecheck_result.type_id);
        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        if (!target_type->find_method(StringPool::get().store("__math_negate"), {typecheck_result.type_id})) {
            const std::string message = std::format("Type '{}' doesn't support unary operator '-'", *target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithDiagnostic, .type_id = typecheck_result.type_id};
        }

        // If the data type supports mathematical negation but the typecheck didn't report a diagnostic we have to propagate that
        // because this ast node doesn't know a proper diagnostic
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithoutDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = typecheck_result.type_id};
        }
        return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = typecheck_result.type_id};
    }

    TypeCheckResult TypeCheckPass::typecheck_variable_expression(VariableExpression* expression, TypeId requested_type_id) const {
        KPL_ASSERT_NOT_NULLPTR(expression);
        KPL_ASSERT_THAT(expression->symbol_id != SymbolId::invalid());
        KPL_ASSERT_THAT(expression->node_type != ASTNodeType::Poison);
        KPL_ASSERT_THAT(requested_type_id != TypeId::invalid());
        const Symbol* symbol = symbol_table.lookup(expression->symbol_id);
        KPL_ASSERT_THAT(symbol->type_id != TypeId::invalid());
        if (requested_type_id == symbol->type_id || requested_type_id == type_table.Builtins.unknown_type_id) {
            return {.status = TypeCheckResult::Status::RequestFulfilled, .type_id = symbol->type_id};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PoisonedWithoutDiagnostic, .type_id = symbol->type_id};
        }
    }
}
