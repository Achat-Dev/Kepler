// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include "emergency.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kepler::parser {

    std::unique_ptr<ast::ASTNode> Parser::parse_statement() {
        switch (current_token->type) {
            case lexer::TokenType::If:
                return parse_if();
            case lexer::TokenType::For:
                return parse_for();
            case lexer::TokenType::Return:
                return parse_return();
            case lexer::TokenType::DataType: {
                return parse_variable_definition();
            }
            case lexer::TokenType::Identifier: {
                const lexer::Token* identifier_token = current_token;
                next_token(true); // eat identifier
                if (current_token->type == lexer::TokenType::Assignment) {
                    return parse_assignment(identifier_token);
                } else if (current_token->type == lexer::TokenType::BracketOpen) {
                    return parse_call(identifier_token);
                }
                previous_token(true); // Previous token for the diagnostic
                break;
            }

            default:
                break;
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<ast::AssignmentStatement> Parser::parse_assignment(const lexer::Token* identifier_token) {
        const diagnostics::SourceLocation& assignment_source_location = current_token->source_location;
        if (current_token->type != lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '=' after identifier in assignment", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '='
        std::unique_ptr<ast::Expression> value_expression = parse_expression();
        if (!value_expression) {
            return nullptr; // parse_expression alredy recovered, so no need to recover here
        }

        const std::string& identifier = std::get<std::string>(identifier_token->data);
        return std::make_unique<ast::AssignmentStatement>(std::make_unique<ast::VariableExpression>(identifier, identifier_token->source_location), std::move(value_expression), assignment_source_location);
    }

    std::unique_ptr<ast::IfStatement> Parser::parse_if() {
        const lexer::Token* if_token = current_token;

        std::unique_ptr<ast::Expression> condition = nullptr;
        next_token(true); // eat 'if' or 'elseif'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            const std::string message = std::format("Expected '(' after '{}'", if_token->type);
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            if (current_token->type == lexer::TokenType::End) {
                next_token(true); // eat 'end'
                return nullptr;
            }
        } else {
            next_token(true); // eat '('
            if (current_token->type == lexer::TokenType::BracketClose) {
                const std::string message = std::format("Missing '{}' condition, expected expression as condition", if_token->type);
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, current_token->source_location);
                next_token(true);
            } else {
                // Parse 'if' condition
                condition = parse_expression();
                if (condition && current_token->type != lexer::TokenType::BracketClose) {
                    const diagnostics::SourceLocation source_location(current_token->source_location.file_id, current_token->source_location.position + 1, 1);
                    const std::string message = std::format("Expected ')' after condition in '{}'", if_token->type);
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, source_location);
                    recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                    if (current_token->type == lexer::TokenType::End) {
                        next_token(true); // eat 'end'
                        return nullptr;
                    }
                }
            }
        }

        if (condition) {
            next_token(true); // eat ')'
        }
        const std::string message = std::format("'{}' statement was not closed with an 'end' keyword", if_token->type);
        auto if_body = parse_body<lexer::TokenType::Elseif, lexer::TokenType::Else, lexer::TokenType::End>(message, if_token->source_location);

        // Setup behaviour for different 'if' ending cases
        switch (current_token->type) {
            case lexer::TokenType::Elseif: {
                // Don' eat the 'elseif', it will be eaten by parse_if
                std::unique_ptr<ast::IfStatement> elseif = parse_if();
                if (!condition || !if_body || !elseif) {
                    return nullptr;
                }
                std::vector<std::unique_ptr<ast::ASTNode>> elseif_body;
                elseif_body.push_back(std::move(elseif));
                return std::make_unique<ast::IfStatement>(std::move(condition), std::move(*if_body), std::move(elseif_body), if_token->source_location);
            }
            case lexer::TokenType::Else: {
                const diagnostics::SourceLocation& source_location = current_token->source_location;
                next_token(true); // eat 'else'
                auto else_body = parse_body<lexer::TokenType::End>("'else' statement was not closed with an 'end' keyword", source_location);
                next_token(true); // eat 'end'
                if (!condition || !if_body || !else_body) {
                    return nullptr;
                }
                return std::make_unique<ast::IfStatement>(std::move(condition), std::move(*if_body), std::move(*else_body), if_token->source_location);
            }
            case lexer::TokenType::End: {
                next_token(true); // eat 'end'
                if (!condition || !if_body) {
                    return nullptr;
                }
                return std::make_unique<ast::IfStatement>(std::move(condition), std::move(*if_body), std::vector<std::unique_ptr<ast::ASTNode>>{}, if_token->source_location);
            }
            default:
                emergency_exit("I may have messed up parsing an if statement so badly that it neither ends with 'elseif', 'else' or 'end' nor have I encountered EOF. Whoops (ᵕ—ᗜ—)");
        }
    }

    std::unique_ptr<ast::ForStatement> Parser::parse_for() {
        const diagnostics::SourceLocation& for_source_location = current_token->source_location;
        next_token(true); // eat 'for'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type == lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Missing 'for' definition, expected at least '(<datatype> <identifier> : <end_value>)'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        } else if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after '(' in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }
        const lexer::Token* data_type_token = current_token;

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        const std::string& identifier = std::get<std::string>(current_token->data);
        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::Colon) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ':' after identifier in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ':'
        std::unique_ptr<ast::Expression> first_value = parse_expression();
        if (!first_value) {
            if (current_token->type == lexer::TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only end value is given, start and step are implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier, data_type_token, nullptr, std::move(first_value), nullptr, for_source_location);
        } else if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after first expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        std::unique_ptr<ast::Expression> end_value = parse_expression();
        if (!end_value) {
            if (current_token->type == lexer::TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only start and end value are given, step is implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier, data_type_token, std::move(first_value), std::move(end_value), nullptr, for_source_location);
        } else if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after second expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        std::unique_ptr<ast::Expression> step_value = parse_expression();
        if (!step_value) {
            if (current_token->type == lexer::TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Start, stop and end values are given
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier, data_type_token, std::move(first_value), std::move(end_value), std::move(step_value), for_source_location);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after third expression in 'for'", current_token->source_location);
        recover_for_definition_and_parse_body(for_source_location);
        return nullptr;
    }

    std::unique_ptr<ast::ForStatement> Parser::create_for_statement(const std::string& variable_identifier, const lexer::Token* variable_data_type_token, std::unique_ptr<ast::Expression> start_value, std::unique_ptr<ast::Expression> end_value, std::unique_ptr<ast::Expression> step_value, const diagnostics::SourceLocation& for_source_location) {
        next_token(true); // eat ')'
        auto body = parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
        next_token(true); // eat 'end'
        if (!body) {
            return nullptr;
        }

        const type_system::DataTypeKind variable_data_type = std::get<type_system::DataTypeKind>(variable_data_type_token->data);
        std::unique_ptr<ast::VariableExpression> variable = std::make_unique<ast::VariableExpression>(variable_identifier, variable_data_type_token->source_location);
        std::unique_ptr<ast::AssignmentStatement> assignment_statement = std::make_unique<ast::AssignmentStatement>(std::move(variable), std::move(start_value), variable_data_type_token->source_location);
        std::unique_ptr<ast::VariableDefinitionStatement> variable_definition_statement = std::make_unique<ast::VariableDefinitionStatement>(variable_data_type, variable_identifier, std::move(assignment_statement), variable_data_type_token->source_location);

        return std::make_unique<ast::ForStatement>(std::move(variable_definition_statement), std::move(end_value), std::move(step_value), std::move(*body), for_source_location);
    }

    void Parser::recover_for_definition_and_parse_body(const diagnostics::SourceLocation& source_location) {
        recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        if (current_token->type == lexer::TokenType::End) {
            next_token(true); // eat 'end'
        } else {
            // Parse the for body to collect diagnostics for it
            parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", source_location);
        }
    }

    std::unique_ptr<ast::ReturnStatement> Parser::parse_return() {
        const diagnostics::SourceLocation& return_source_location = current_token->source_location;
        next_token(true); // eat 'return' keyword

        switch (current_token->type) {
            case lexer::TokenType::If:
            case lexer::TokenType::For:
            case lexer::TokenType::Return: {
                const std::string message = std::format("Cannot return '{}' statement", current_token->type);
                diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidReturnExpression, message, current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return nullptr;
            }
            case lexer::TokenType::DataType:
                next_token(true); // eat data type
                // It's a cast if there is a open bracket next, which is allowed here, so break out if that's the case
                if (current_token->type == lexer::TokenType::BracketOpen) {
                    previous_token(true);
                    break;
                }
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}' (data types can only be used for casting here)", *current_token), current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return nullptr;
            case lexer::TokenType::End:
                return std::make_unique<ast::ReturnStatement>(nullptr, return_source_location);
            default:
                break;
        }

        std::unique_ptr<ast::Expression> expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression already recovered, so no need to recover here
        }

        return std::make_unique<ast::ReturnStatement>(std::move(expression), return_source_location);
    }

    std::unique_ptr<ast::VariableDefinitionStatement> Parser::parse_variable_definition() {
        const diagnostics::SourceLocation& data_type_source_location = current_token->source_location;
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier (data types can only be used for variable definitions here)", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        const lexer::Token* identifier_token = current_token;
        next_token(true); // eat identifier
        std::unique_ptr<ast::AssignmentStatement> assignment_statement = parse_assignment(identifier_token);
        if (!assignment_statement) {
            return nullptr; // parse_assignment already recovered, so no need to recover here
        }

        if (data_type == type_system::DataTypeKind::Void) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidVariableType, "Cannot create a local variable of type 'void'", data_type_source_location);
            return nullptr;
        }

        const std::string& identifier = std::get<std::string>(identifier_token->data);
        return std::make_unique<ast::VariableDefinitionStatement>(data_type, identifier, std::move(assignment_statement), data_type_source_location);
    }

}
