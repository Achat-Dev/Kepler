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
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    using Tcr = TypeCheckPass::TypeCheckResult;

    void TypeCheckPass::run() {
        typecheck_nodes(ast.nodes);
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

        assert(false && "Missing boolean operator check implementation");
        std::unreachable();
    }

    Tcr TypeCheckPass::typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        Tcr::Status result_status = Tcr::Status::RequestFulfilled;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            const Tcr typecheck_result = typecheck_node(node.get(), *type_table.Builtins.unknown_type);
            if (typecheck_result.is_poisoned()) {
                result_status = Tcr::Status::PoisonedWithDiagnostic;
            }
        }
        return {.status = result_status, .type = nullptr};
    }

    Tcr TypeCheckPass::typecheck_node(ASTNode* node, Type& requested_type) {
        assert(node != nullptr);
        if (node->node_type == ASTNodeType::Poison) {
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        switch (node->node_type) {
            case ASTNodeType::Extern:
                return {.status = Tcr::Status::RequestFulfilled, .type = nullptr};
            case ASTNodeType::Function:
                typecheck_function(static_cast<Function*>(node));
                return {.status = Tcr::Status::RequestFulfilled, .type = nullptr};
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
            default:
                assert(false && "Failed to typecheck ast node");
                std::unreachable();
        }
    }

    void TypeCheckPass::typecheck_function(Function* function) {
        current_function_return_type = function->prototype->return_type;
        // Typecheck the body without poisoning the function so that the unpoisoned part of the body can still be accessed by later passes
        typecheck_nodes(function->body);
        current_function_return_type = nullptr;
    }

    Tcr TypeCheckPass::typecheck_assignment_statement(AssignmentStatement* statement) {
        const Symbol* variable_symbol = symbol_table.lookup(statement->variable_expression->identifier_id);
        const std::string_view variable_name = StringPool::get().lookup(statement->variable_expression->identifier_id);
        assert(variable_symbol != nullptr);
        assert(variable_symbol->type != nullptr);

        const Tcr typecheck_result = typecheck_node(statement->value_expression.get(), *variable_symbol->type);
        if (typecheck_result.status == Tcr::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return typecheck_result;
        } else if (typecheck_result.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: cannot assign a value of type '{}' to a variable of type '{}'",
                typecheck_result.type,
                variable_symbol->type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }

        return {.status = Tcr::Status::RequestFulfilled, .type = variable_symbol->type};
    }

    Tcr TypeCheckPass::typecheck_for_statement(ForStatement* statement) {
        Type* variable_type = statement->loop_variable_definition->type;
        assert(variable_type != nullptr);
        if (!is_integer_type(variable_type->type_kind)) {
            const std::string message = std::format("Loop variable of a for statement has to be an integer type, got '{}'", variable_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->loop_variable_definition->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        const Tcr variable_tcr = typecheck_variable_definition_statement(statement->loop_variable_definition.get());
        if (variable_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }

        const Tcr end_tcr = typecheck_node(statement->end_value.get(), *variable_type);
        if (end_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        } else if (end_tcr.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", variable_type, end_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->end_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        // Default value for the typecheck result of the step_value in case the step doesn't exist
        Tcr step_tcr{.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
        if (statement->step_value != nullptr) {
            step_tcr = typecheck_node(statement->step_value.get(), *variable_type);
        }
        if (step_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
            return typecheck_body_and_poison_for_statement(statement);
        }
        if (step_tcr.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", variable_type, step_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->step_value->source_location);
            return typecheck_body_and_poison_for_statement(statement);
        }

        const Tcr body_tcr = typecheck_nodes(statement->body);
        if (variable_tcr.is_poisoned() || end_tcr.is_poisoned() || step_tcr.is_poisoned() || body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    Tcr TypeCheckPass::typecheck_body_and_poison_for_statement(ForStatement* statement) {
        statement->node_type = ASTNodeType::Poison;
        typecheck_nodes(statement->body);
        return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
    }

    Tcr TypeCheckPass::typecheck_if_statement(IfStatement* statement) {
        const Tcr condition_tcr = typecheck_node(statement->condition.get(), *type_table.Builtins.bool_type);
        if (condition_tcr.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Condition of an if statement needs to be of type bool, got '{}'", condition_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->condition->source_location);
        }

        const Tcr if_body_tcr = typecheck_nodes(statement->if_body);
        const Tcr else_body_tcr = typecheck_nodes(statement->else_body);
        if (condition_tcr.is_poisoned() || if_body_tcr.is_poisoned() || else_body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    Tcr TypeCheckPass::typecheck_return_statement(ReturnStatement* statement) {
        assert(current_function_return_type != nullptr);
        assert(current_function_return_type != type_table.Builtins.unknown_type);

        if (current_function_return_type == type_table.Builtins.void_type && statement->expression != nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn, "Cannot return a value from void function", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        } else if (current_function_return_type != type_table.Builtins.void_type && statement->expression == nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn,
                "Expected expression after 'return' (non-void function needs a return value)",
                statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        Tcr typecheck_result = typecheck_node(statement->expression.get(), *current_function_return_type);
        if (typecheck_result.status == Tcr::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        } else if (typecheck_result.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: cannot return a value of type '{}' from a function with a return type of '{}'", typecheck_result.type, current_function_return_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    Tcr TypeCheckPass::typecheck_variable_definition_statement(VariableDefinitionStatement* statement) {
        assert(statement->type != nullptr);
        const Tcr typecheck_result = typecheck_assignment_statement(statement->assignment_statement.get());
        assert(typecheck_result.status != Tcr::Status::PoisonedWithoutDiagnostic);
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .type = statement->type};
    }

    Tcr TypeCheckPass::typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, const Type& requested_type) const {
        if (&requested_type == type_table.Builtins.bool_type || &requested_type == type_table.Builtins.unknown_type) {
            return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.bool_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.bool_type};
        }
    }

    Tcr TypeCheckPass::typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, Type& requested_type) const {
        if (is_floating_point_type(requested_type.type_kind)) {
            expression->target_type = &requested_type;
            return {.status = Tcr::Status::RequestFulfilled, .type = &requested_type};
        } else if (&requested_type == type_table.Builtins.unknown_type) {
            expression->target_type = type_table.Builtins.f32_type;
            return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.f32_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.unknown_type};
        }
    }

    Tcr TypeCheckPass::typecheck_integer_literal_expression(IntegerLiteralExpression* expression, Type& requested_type) const {
        if (is_integer_type(requested_type.type_kind) || is_floating_point_type(requested_type.type_kind)) {
            expression->target_type = &requested_type;
            return {.status = Tcr::Status::RequestFulfilled, .type = &requested_type};
        } else if (&requested_type == type_table.Builtins.unknown_type) {
            expression->target_type = type_table.Builtins.i32_type;
            return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.i32_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.unknown_type};
        }
    }

    Tcr TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, const Type& requested_type) const {
        if (&requested_type == type_table.Builtins.string_type || &requested_type == type_table.Builtins.unknown_type) {
            return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.string_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = type_table.Builtins.string_type};
        }
    }

    Tcr TypeCheckPass::typecheck_binary_expression(BinaryExpression* expression, Type& requested_type) {
        Tcr lhs_tcr;
        Tcr rhs_tcr;

        if (&requested_type == type_table.Builtins.bool_type) {
            if (!is_boolean_operator(expression->operator_type)) {
                const std::string message = std::format("Type mismatch: Binary operator '{}' doesn't produce a boolean value", expression->operator_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }

            // Pass unknown as the requested type so that the expressions don't try to produce a boolean individually
            lhs_tcr = typecheck_node(expression->lhs.get(), *type_table.Builtins.unknown_type);
            assert(lhs_tcr.status != Tcr::Status::PoisonedWithoutDiagnostic);
            if (lhs_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }

            rhs_tcr = typecheck_node(expression->rhs.get(), *type_table.Builtins.unknown_type);
            assert(rhs_tcr.status != Tcr::Status::PoisonedWithoutDiagnostic);
            if (rhs_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
            }
        } else {
            lhs_tcr = typecheck_binary_expression_side(expression, expression->lhs.get(), requested_type);
            assert(lhs_tcr.status != Tcr::Status::PoisonedWithoutDiagnostic);
            if (lhs_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
                return lhs_tcr;
            }

            rhs_tcr = typecheck_binary_expression_side(expression, expression->rhs.get(), requested_type);
            assert(rhs_tcr.status != Tcr::Status::PoisonedWithoutDiagnostic);
            if (rhs_tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
                return rhs_tcr;
            }
        }

        const StringId operator_name_id = get_operator_name_id(expression->operator_type);
        if (!lhs_tcr.type->find_method(operator_name_id, {rhs_tcr.type})) {
            const std::string message = std::format("No implemention of binary operator '{}' between types '{}' and '{}'",
                expression->operator_type,
                lhs_tcr.type,
                rhs_tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        return {.status = Tcr::Status::RequestFulfilled, .type = type_table.Builtins.unknown_type};
    }

    Tcr TypeCheckPass::typecheck_binary_expression_side(BinaryExpression* binary_expression, Expression* side_expression, Type& requested_type) {
        const Tcr tcr = typecheck_node(side_expression, requested_type);
        if (tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        } else if (tcr.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", requested_type, tcr.type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), side_expression->source_location);
            binary_expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .type = &requested_type};
    }

    Tcr TypeCheckPass::typecheck_call_expression(CallExpression* expression, const Type& requested_type) {
        const Symbol* prototype_symbol = symbol_table.lookup(expression->identifier_id);
        assert(prototype_symbol != nullptr);
        if (&requested_type != prototype_symbol->type && &requested_type != type_table.Builtins.unknown_type) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = prototype_symbol->type};
        }

        const std::vector<Type*>& parameter_types = std::get<PrototypeSymbolData>(prototype_symbol->data).parameter_types;
        assert(parameter_types.size() == expression->args.size() && "Mismatching call parameter count during typechecking (should have been caught by the name resolution)");

        for (size_t i = 0; i < parameter_types.size(); i++) {
            const Tcr typecheck_result = typecheck_node(expression->args[i].get(), *parameter_types[i]);
            if (typecheck_result.status == Tcr::Status::PoisonedWithDiagnostic) {
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = prototype_symbol->type};
            } else if (typecheck_result.status == Tcr::Status::PoisonedWithoutDiagnostic) {
                // Nodes only poison themselves without diagnostics if they aren't able to produce the requested data type, hence the assert
                assert(typecheck_result.type != parameter_types[i]);
                const std::string_view identifier = StringPool::get().lookup(prototype_symbol->identifier_id);
                const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' expects type '{}', but the given argument is of type '{}'",
                    (i + 1),
                    identifier,
                    parameter_types[i],
                    typecheck_result.type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, expression->args[i]->source_location);
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = prototype_symbol->type};
            }
        }

        return {.status = Tcr::Status::RequestFulfilled, .type = prototype_symbol->type};
    }

    Tcr TypeCheckPass::typecheck_cast_expression(CastExpression* expression, const Type& requested_type) {
        if (&requested_type != type_table.Builtins.unknown_type && &requested_type != expression->target_type) {
            const std::string message = std::format("Type mismatch: Expected '{}', got '{}'", requested_type, expression->target_type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        const Tcr tcr = typecheck_node(expression->expression.get(), *type_table.Builtins.unknown_type);
        assert(tcr.status != Tcr::Status::PoisonedWithoutDiagnostic);
        if (tcr.status == Tcr::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        if (!expression->target_type->find_method(StringPool::get().store("new"), {tcr.type})) {
            const std::string message = std::format("Type '{}' doesn't implement cast to type '{}'", expression->target_type, tcr.type);
            diagnostic_sink.report(DiagnosticCode::InvalidCast, std::move(message), expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = type_table.Builtins.unknown_type};
        }

        if (expression->target_type == tcr.type) {
            const std::string message = std::format("Redundant cast of type '{}' to itself (the cast will be discarded)", tcr.type);
            diagnostic_sink.report(DiagnosticCode::RedundantCast, std::move(message), expression->source_location);
        }

        return {.status = Tcr::Status::RequestFulfilled, .type = expression->target_type};
    }

    Tcr TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, Type& requested_type) {
        Tcr typecheck_result = typecheck_node(expression->expression.get(), requested_type);
        if (typecheck_result.status == Tcr::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }

        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        if (typecheck_result.type->find_method(StringPool::get().store("__math_negate"), {typecheck_result.type})) {
            const std::string message = std::format("Type '{}' doesn't support unary operator '-'", typecheck_result.type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .type = typecheck_result.type};
        }

        // If the data type supports mathematical negation but the typecheck didn't report a diagnostic we have to propagate that
        // because this ast node doesn't know a proper diagnostic
        if (typecheck_result.status == Tcr::Status::PoisonedWithoutDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = typecheck_result.type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .type = typecheck_result.type};
    }

    Tcr TypeCheckPass::typecheck_variable_expression(VariableExpression* expression, const Type& requested_type) const {
        const Symbol* symbol = symbol_table.lookup(expression->identifier_id);
        if (&requested_type == symbol->type || &requested_type == type_table.Builtins.unknown_type) {
            return {.status = Tcr::Status::RequestFulfilled, .type = symbol->type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithoutDiagnostic, .type = symbol->type};
        }
    }
}
