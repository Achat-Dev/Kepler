// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/type_check_pass.hpp"
#include "assert.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
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
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
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
    }

    Tcr TypeCheckPass::typecheck_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes) {
        Tcr::Status result_status = Tcr::Status::RequestFulfilled;
        for (const std::unique_ptr<ASTNode>& node : nodes) {
            const Tcr typecheck_result = typecheck_node(node.get(), DataTypeKind::Internal_Undetermined);
            if (typecheck_result.is_poisoned()) {
                result_status = Tcr::Status::PoisonedWithDiagnostic;
            }
        }
        return {.status = result_status, .data_type = DataTypeKind::Internal_Undetermined};
    }

    Tcr TypeCheckPass::typecheck_node(ASTNode* node, DataTypeKind requested_data_type) {
        if (node == nullptr) {
            return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Internal_Undetermined};
        }

        if (node->node_type == ASTNodeType::Poison) {
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }

        switch (node->node_type) {
            case ASTNodeType::Extern:
                return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Internal_Undetermined};
            case ASTNodeType::Function:
                typecheck_function(static_cast<const Function*>(node));
                return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Internal_Undetermined};
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
                return typecheck_boolean_literal_expression(static_cast<BooleanLiteralExpression*>(node), requested_data_type);
            case ASTNodeType::FloatingPointLiteralExpression:
                return typecheck_floating_point_literal_expression(static_cast<FloatingPointLiteralExpression*>(node), requested_data_type);
            case ASTNodeType::IntegerLiteralExpression:
                return typecheck_integer_literal_expression(static_cast<IntegerLiteralExpression*>(node), requested_data_type);
            case ASTNodeType::StringLiteralExpression:
                return typecheck_string_literal_expression(static_cast<StringLiteralExpression*>(node), requested_data_type);
            case ASTNodeType::BinaryExpression:
                return typecheck_binary_expression(static_cast<BinaryExpression*>(node), requested_data_type);
            case ASTNodeType::CallExpression:
                return typecheck_call_expression(static_cast<CallExpression*>(node), requested_data_type);
            case ASTNodeType::CastExpression:
                return typecheck_cast_expression(static_cast<CastExpression*>(node), requested_data_type);
            case ASTNodeType::MathematicalNegationExpression:
                return typecheck_mathematical_negation_expression(static_cast<MathematicalNegationExpression*>(node), requested_data_type);
            case ASTNodeType::VariableExpression:
                return typecheck_variable_expression(static_cast<VariableExpression*>(node), requested_data_type);
            default:
                KPL_ASSERT(false, "I have no idea how I am supposed to typecheck an ast node of type '{}'", node->node_type);
                std::unreachable();
        }
    }

    void TypeCheckPass::typecheck_function(const Function* function) {
        current_function_return_type = function->prototype->return_type;
        typecheck_nodes(function->body);
        current_function_return_type = DataTypeKind::Internal_Undetermined;
    }

    Tcr TypeCheckPass::typecheck_assignment_statement(AssignmentStatement* statement) {
        const Symbol* variable_symbol = symbol_table.lookup(statement->variable_expression->identifier_id);
        const Tcr typecheck_result = typecheck_node(statement->value_expression.get(), variable_symbol->data_type);
        switch (typecheck_result.status) {
            case Tcr::Status::PoisonedWithDiagnostic:
                statement->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = typecheck_result.data_type};
            case Tcr::Status::PosionedWithoutDiagnostic: {
                const std::string message = std::format("Type mismatch: cannot assign a value of type '{}' to a variable of type '{}'",
                    typecheck_result.data_type, variable_symbol->data_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->source_location);
                statement->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
            }
            case Tcr::Status::RequestFulfilled:
                return {.status = Tcr::Status::RequestFulfilled, .data_type = variable_symbol->data_type};
        }
    }

    Tcr TypeCheckPass::typecheck_for_statement(ForStatement* statement) {
        const DataTypeKind variable_data_type = statement->loop_variable_definition->data_type;
        if (!is_integer_type(variable_data_type)) {
            const std::string message = std::format("Loop variable of a for statement has to be an integer type, got '{}'",
                statement->loop_variable_definition->data_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->loop_variable_definition->source_location);
            statement->node_type = ASTNodeType::Poison;
            typecheck_nodes(statement->body);
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }

        const Tcr variable_tcr = typecheck_variable_definition_statement(statement->loop_variable_definition.get());
        const Tcr end_tcr = typecheck_node(statement->end_value.get(), variable_data_type);
        if (end_tcr.status == Tcr::Status::PosionedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: expected '{}', got '{}'", variable_data_type, end_tcr.data_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->end_value->source_location);
            statement->node_type = ASTNodeType::Poison;
            typecheck_nodes(statement->body);
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }

        const Tcr step_tcr = typecheck_node(statement->step_value.get(), variable_data_type);
        if (step_tcr.status == Tcr::Status::PosionedWithoutDiagnostic) {
            const std::string message = std::format("Type mismatch: expected '{}', got '{}'", variable_data_type, step_tcr.data_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->step_value->source_location);
            statement->node_type = ASTNodeType::Poison;
            typecheck_nodes(statement->body);
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }

        const Tcr body_tcr = typecheck_nodes(statement->body);
        if (variable_tcr.is_poisoned() || end_tcr.is_poisoned() || step_tcr.is_poisoned() || body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }
        return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Internal_Undetermined};
    }

    Tcr TypeCheckPass::typecheck_if_statement(IfStatement* statement) {
        const Tcr condition_tcr = typecheck_node(statement->condition.get(), DataTypeKind::Bool);
        if (condition_tcr.status == Tcr::Status::PosionedWithoutDiagnostic) {
            const std::string message = std::format("Condition of an if statement needs to be of type bool, got '{}'", condition_tcr.data_type);
            diagnostic_sink.report(DiagnosticCode::TypeMismatch, std::move(message), statement->condition->source_location);
            statement->node_type = ASTNodeType::Poison;
            typecheck_nodes(statement->if_body);
            typecheck_nodes(statement->else_body);
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }

        const Tcr if_body_tcr = typecheck_nodes(statement->if_body);
        const Tcr else_body_tcr = typecheck_nodes(statement->else_body);
        if (condition_tcr.is_poisoned() || if_body_tcr.is_poisoned() || else_body_tcr.is_poisoned()) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }
        return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Internal_Undetermined};
    }

    Tcr TypeCheckPass::typecheck_return_statement(ReturnStatement* statement) {
        KPL_ASSERT(current_function_return_type != DataTypeKind::Internal_Undetermined, "Someone - and I am not going to say who (maybe because it was myself) - forgot to store the return type of the currently typechecked function. And maybe, but just maybe, that's the reason why I'm currently shitting myself trying to parse a return statement without knowing the return type of the function.");

        if (current_function_return_type == DataTypeKind::Void && statement->expression != nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn, "Cannot return a value from void function", statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Void};
        } else if (current_function_return_type != DataTypeKind::Void && statement->expression == nullptr) {
            diagnostic_sink.report(DiagnosticCode::InvalidReturn,
                "Expected expression after 'return' (non-void function needs a return value)",
                statement->source_location);
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = DataTypeKind::Void};
        }

        Tcr typecheck_result = typecheck_node(statement->expression.get(), current_function_return_type);
        switch (typecheck_result.status) {
            case Tcr::Status::PoisonedWithDiagnostic:
                statement->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
            case Tcr::Status::PosionedWithoutDiagnostic: {
                const std::string message = std::format("Type mismatch: cannot return a value of type '{}' from a function with a return type of '{}'", typecheck_result.data_type, current_function_return_type);
                diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, statement->source_location);
                statement->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
            }
            case Tcr::Status::RequestFulfilled:
                return {.status = Tcr::Status::RequestFulfilled, .data_type = current_function_return_type};
        }
    }

    Tcr TypeCheckPass::typecheck_variable_definition_statement(VariableDefinitionStatement* statement) {
        const Tcr typecheck_result = typecheck_assignment_statement(statement->assignment_statement.get());
        KPL_ASSERT(typecheck_result.status != Tcr::Status::PosionedWithoutDiagnostic,
            "Assignment statement of a variable definition has to report a disgnostic if it poisons itself");
        if (typecheck_result.status == TypeCheckResult::Status::PoisonedWithDiagnostic) {
            statement->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .data_type = statement->data_type};
    }

    Tcr TypeCheckPass::typecheck_boolean_literal_expression(BooleanLiteralExpression* expression, DataTypeKind requested_data_type) const {
        switch (requested_data_type) {
            case DataTypeKind::Bool:
            case DataTypeKind::Internal_Undetermined:
                return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::Bool};
            default:
                expression->node_type = ASTNodeType::Poison;
                return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = DataTypeKind::Bool};
        }
    }

    Tcr TypeCheckPass::typecheck_floating_point_literal_expression(FloatingPointLiteralExpression* expression, DataTypeKind requested_data_type) const {
        if (is_floating_point_type(requested_data_type)) {
            expression->target_type = requested_data_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .data_type = requested_data_type};
        } else if (requested_data_type == DataTypeKind::Internal_Undetermined) {
            expression->target_type = DataTypeKind::Float32;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .data_type = DataTypeKind::Float32};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PosionedWithoutDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }
    }

    Tcr TypeCheckPass::typecheck_integer_literal_expression(IntegerLiteralExpression* expression, DataTypeKind requested_data_type) const {
        if (is_integer_type(requested_data_type)) {
            expression->target_type = requested_data_type;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .data_type = requested_data_type};
        } else if (requested_data_type == DataTypeKind::Internal_Undetermined) {
            expression->target_type = DataTypeKind::Int32;
            return {.status = TypeCheckResult::Status::RequestFulfilled, .data_type = DataTypeKind::Int32};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = TypeCheckResult::Status::PosionedWithoutDiagnostic, .data_type = DataTypeKind::Internal_Undetermined};
        }
    }

    Tcr TypeCheckPass::typecheck_string_literal_expression(StringLiteralExpression* expression, DataTypeKind requested_data_type) const {
        if (requested_data_type == DataTypeKind::String) {
            return {.status = Tcr::Status::RequestFulfilled, .data_type = DataTypeKind::String};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = DataTypeKind::String};
        }
    }

    Tcr TypeCheckPass::typecheck_binary_expression(BinaryExpression* expression, DataTypeKind requested_data_type) {
    }

    Tcr TypeCheckPass::typecheck_call_expression(CallExpression* expression, DataTypeKind requested_data_type) {
        const Symbol* prototype_symbol = symbol_table.lookup(expression->identifier_id);
        if (requested_data_type != prototype_symbol->data_type && requested_data_type != DataTypeKind::Internal_Undetermined) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = prototype_symbol->data_type};
        }

        const std::vector<DataTypeKind>& parameter_data_types = std::get<PrototypeSymbolData>(prototype_symbol->data).parameter_data_types;
        KPL_ASSERT(parameter_data_types.size() == expression->args.size(),
            "Argument count mismatch during typechecking, this should have been caught during semantic analysis");

        for (size_t i = 0; i < parameter_data_types.size(); i++) {
            const Tcr typecheck_result = typecheck_node(expression->args[i].get(), parameter_data_types[i]);
            switch (typecheck_result.status) {
                case Tcr::Status::PoisonedWithDiagnostic:
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = prototype_symbol->data_type};
                case Tcr::Status::PosionedWithoutDiagnostic: {
                    // Nodes only poison themselves without diagnostics if they aren't able to produce the requested data type, hence the assert
                    KPL_ASSERT(typecheck_result.data_type != parameter_data_types[i],
                        "For some reason an argument in a call expression poisoned itself without reporting a diagnostic while producing the requested data type");

                    const std::string_view identifier = StringPool::get().lookup(prototype_symbol->identifier_id);
                    const std::string message = std::format("Type mismatch: Parameter no. {} of function '{}' expects type '{}', but the given argument is of type '{}'",
                        (i + 1),
                        identifier,
                        parameter_data_types[i],
                        typecheck_result.data_type);
                    diagnostic_sink.report(DiagnosticCode::TypeMismatch, message, expression->args[i]->source_location);
                    expression->node_type = ASTNodeType::Poison;
                    return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = prototype_symbol->data_type};
                }
                default:
                    break;
            }
        }
        return {.status = Tcr::Status::RequestFulfilled, .data_type = prototype_symbol->data_type};
    }

    Tcr TypeCheckPass::typecheck_cast_expression(CastExpression* expression, DataTypeKind requested_data_type) const {
    }

    Tcr TypeCheckPass::typecheck_mathematical_negation_expression(MathematicalNegationExpression* expression, DataTypeKind requested_data_type) {
        Tcr typecheck_result = typecheck_node(expression->expression.get(), requested_data_type);
        if (typecheck_result.status == Tcr::Status::PoisonedWithDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
        }

        // The data type of the expression to negate maybe doesn't support mathematical negation
        // This can be the case either if the typecheck fulfills the requested data type or the typechecks fails but doesn't report a diagnostic
        if (!is_integer_type(typecheck_result.data_type) && !is_floating_point_type(typecheck_result.data_type)) {
            const std::string message = std::format("Cannot mathematically negate an expression of type '{}'", typecheck_result.data_type);
            diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, expression->source_location);
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PoisonedWithDiagnostic, .data_type = typecheck_result.data_type};
        }

        // If the data type supports mathematical negation but the typecheck didn't report a diagnostic we have to propagate that
        // because this ast node doesn't know a proper diagnostic
        if (typecheck_result.status == Tcr::Status::PosionedWithoutDiagnostic) {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = typecheck_result.data_type};
        }
        return {.status = Tcr::Status::RequestFulfilled, .data_type = typecheck_result.data_type};
    }

    Tcr TypeCheckPass::typecheck_variable_expression(VariableExpression* expression, DataTypeKind requested_data_type) const {
        const Symbol* symbol = symbol_table.lookup(expression->identifier_id);
        if (requested_data_type == symbol->data_type || requested_data_type == DataTypeKind::Internal_Undetermined) {
            return {.status = Tcr::Status::RequestFulfilled, .data_type = symbol->data_type};
        } else {
            expression->node_type = ASTNodeType::Poison;
            return {.status = Tcr::Status::PosionedWithoutDiagnostic, .data_type = symbol->data_type};
        }
    }

}
