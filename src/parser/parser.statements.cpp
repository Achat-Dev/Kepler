// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "assert.hpp"
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
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    std::unique_ptr<ASTNode> Parser::parse_statement() {
        switch (current_token->type) {
            case TokenType::If:
                return parse_if();
            case TokenType::For:
                return parse_for();
            case TokenType::Return:
                return parse_return();
            case TokenType::DataType: {
                return parse_variable_definition();
            }
            case TokenType::Identifier: {
                const Token* identifier_token = current_token;
                next_token(true); // eat identifier
                if (current_token->type == TokenType::Assignment) {
                    return parse_assignment(identifier_token);
                } else if (current_token->type == TokenType::BracketOpen) {
                    return parse_call(identifier_token);
                }
                previous_token(true); // Previous token for the diagnostic
                break;
            }

            default:
                break;
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<AssignmentStatement> Parser::parse_assignment(const Token* identifier_token) {
        const SourceLocation& assignment_source_location = current_token->source_location;
        if (current_token->type != TokenType::Assignment) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '=' after identifier in assignment", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '='
        std::unique_ptr<Expression> value_expression = parse_expression();
        if (!value_expression) {
            return nullptr; // parse_expression alredy recovered, so no need to recover here
        }

        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        return std::make_unique<AssignmentStatement>(std::make_unique<VariableExpression>(identifier_id, identifier_token->source_location),
            std::move(value_expression),
            assignment_source_location);
    }

    std::unique_ptr<IfStatement> Parser::parse_if() {
        const Token* if_token = current_token;

        std::unique_ptr<Expression> condition = nullptr;
        next_token(true); // eat 'if' or 'elseif'
        if (current_token->type != TokenType::BracketOpen) {
            const std::string message = std::format("Expected '(' after '{}'", if_token->type);
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            if (current_token->type == TokenType::End) {
                next_token(true); // eat 'end'
                return nullptr;
            }
        } else {
            next_token(true); // eat '('
            if (current_token->type == TokenType::BracketClose) {
                const std::string message = std::format("Missing '{}' condition, expected expression as condition", if_token->type);
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, current_token->source_location);
                next_token(true);
            } else {
                // Parse 'if' condition
                condition = parse_expression();
                if (condition && current_token->type != TokenType::BracketClose) {
                    const SourceLocation source_location(current_token->source_location.file_id, current_token->source_location.position + 1, 1);
                    const std::string message = std::format("Expected ')' after condition in '{}'", if_token->type);
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, source_location);
                    recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                    if (current_token->type == TokenType::End) {
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
        auto if_body = parse_body<TokenType::Elseif, TokenType::Else, TokenType::End>(message, if_token->source_location);

        // Setup behaviour for different 'if' ending cases
        switch (current_token->type) {
            case TokenType::Elseif: {
                // Don' eat the 'elseif', it will be eaten by parse_if
                std::unique_ptr<IfStatement> elseif = parse_if();
                if (!condition || !if_body || !elseif) {
                    return nullptr;
                }
                std::vector<std::unique_ptr<ASTNode>> elseif_body;
                elseif_body.push_back(std::move(elseif));
                return std::make_unique<IfStatement>(std::move(condition), std::move(*if_body), std::move(elseif_body), if_token->source_location);
            }
            case TokenType::Else: {
                const SourceLocation& source_location = current_token->source_location;
                next_token(true); // eat 'else'
                auto else_body = parse_body<TokenType::End>("'else' statement was not closed with an 'end' keyword", source_location);
                next_token(true); // eat 'end'
                if (!condition || !if_body || !else_body) {
                    return nullptr;
                }
                return std::make_unique<IfStatement>(std::move(condition), std::move(*if_body), std::move(*else_body), if_token->source_location);
            }
            case TokenType::End: {
                next_token(true); // eat 'end'
                if (!condition || !if_body) {
                    return nullptr;
                }
                return std::make_unique<IfStatement>(std::move(condition), std::move(*if_body), std::vector<std::unique_ptr<ASTNode>>{}, if_token->source_location);
            }
            default:
                KPL_ASSERT(false, "I may have messed up parsing an if statement so badly that it neither ends with 'elseif', 'else' or 'end' nor have I encountered EOF. Whoops (ᵕ—ᗜ—)");
                std::unreachable();
        }
    }

    std::unique_ptr<ForStatement> Parser::parse_for() {
        const SourceLocation& for_source_location = current_token->source_location;
        next_token(true); // eat 'for'
        if (current_token->type != TokenType::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type == TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Missing 'for' definition, expected at least '(<datatype> <identifier> : <end_value>)'",
                current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        } else if (current_token->type != TokenType::DataType) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected data type after '(' in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }
        const Token* data_type_token = current_token;

        next_token(true); // eat data type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after data type in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        const StringId identifier_id = std::get<StringId>(current_token->data);
        next_token(true); // eat identifier
        if (current_token->type != TokenType::Colon) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ':' after identifier in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ':'
        std::unique_ptr<Expression> first_value = parse_expression();
        if (!first_value) {
            if (current_token->type == TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only end value is given, start and step are implicit
        if (current_token->type == TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type_token, nullptr, std::move(first_value), nullptr, for_source_location);
        } else if (current_token->type != TokenType::Comma) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after first expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        std::unique_ptr<Expression> end_value = parse_expression();
        if (!end_value) {
            if (current_token->type == TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only start and end value are given, step is implicit
        if (current_token->type == TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type_token, std::move(first_value), std::move(end_value), nullptr, for_source_location);
        } else if (current_token->type != TokenType::Comma) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after second expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        std::unique_ptr<Expression> step_value = parse_expression();
        if (!step_value) {
            if (current_token->type == TokenType::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Start, stop and end values are given
        if (current_token->type == TokenType::BracketClose) {
            return create_for_statement(identifier_id,
                data_type_token,
                std::move(first_value),
                std::move(end_value),
                std::move(step_value),
                for_source_location);
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' after third expression in 'for'", current_token->source_location);
        recover_for_definition_and_parse_body(for_source_location);
        return nullptr;
    }

    // clang-format off
    std::unique_ptr<ForStatement> Parser::create_for_statement(StringId variable_identifier_id,
        const Token* variable_data_type_token,
        std::unique_ptr<Expression> start_value,
        std::unique_ptr<Expression> end_value,
        std::unique_ptr<Expression> step_value,
        SourceLocation for_source_location
    ) {
        // clang-format on
        next_token(true); // eat ')'
        auto body = parse_body<TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
        next_token(true); // eat 'end'
        if (!body) {
            return nullptr;
        }

        const DataTypeKind variable_data_type = std::get<DataTypeKind>(variable_data_type_token->data);
        std::unique_ptr<VariableExpression> variable = std::make_unique<VariableExpression>(variable_identifier_id, variable_data_type_token->source_location);
        std::unique_ptr<AssignmentStatement> assignment_statement = std::make_unique<AssignmentStatement>(std::move(variable),
            std::move(start_value),
            variable_data_type_token->source_location);
        std::unique_ptr<VariableDefinitionStatement> variable_definition_statement = std::make_unique<VariableDefinitionStatement>(variable_data_type,
            variable_identifier_id,
            std::move(assignment_statement),
            variable_data_type_token->source_location);

        return std::make_unique<ForStatement>(std::move(variable_definition_statement),
            std::move(end_value),
            std::move(step_value),
            std::move(*body),
            for_source_location);
    }

    void Parser::recover_for_definition_and_parse_body(SourceLocation source_location) {
        recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
        if (current_token->type == TokenType::End) {
            next_token(true); // eat 'end'
        } else {
            // Parse the for body to collect diagnostics for it
            parse_body<TokenType::End>("'for' statement was not closed with an 'end' keyword", source_location);
        }
    }

    std::unique_ptr<ReturnStatement> Parser::parse_return() {
        const SourceLocation& return_source_location = current_token->source_location;
        next_token(true); // eat 'return' keyword

        switch (current_token->type) {
            case TokenType::If:
            case TokenType::For:
            case TokenType::Return: {
                const std::string message = std::format("Cannot return '{}' statement", current_token->type);
                diagnostic_sink.report(DiagnosticCode::InvalidReturnExpression, message, current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            }
            case TokenType::DataType:
                next_token(true); // eat data type
                // It's a cast if there is a open bracket next, which is allowed here, so break out if that's the case
                if (current_token->type == TokenType::BracketOpen) {
                    previous_token(true);
                    break;
                }
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                    std::format("Unexpected token '{}' (data types can only be used for casting here)", *current_token),
                    current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            case TokenType::End:
                return std::make_unique<ReturnStatement>(nullptr, return_source_location);
            default:
                break;
        }

        std::unique_ptr<Expression> expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression already recovered, so no need to recover here
        }

        return std::make_unique<ReturnStatement>(std::move(expression), return_source_location);
    }

    std::unique_ptr<VariableDefinitionStatement> Parser::parse_variable_definition() {
        const SourceLocation& data_type_source_location = current_token->source_location;
        const DataTypeKind data_type = std::get<DataTypeKind>(current_token->data);

        next_token(true); // eat data type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Expected identifier (data types can only be used for variable definitions here)",
                current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        const Token* identifier_token = current_token;
        next_token(true); // eat identifier
        std::unique_ptr<AssignmentStatement> assignment_statement = parse_assignment(identifier_token);
        if (!assignment_statement) {
            return nullptr; // parse_assignment already recovered, so no need to recover here
        }

        if (data_type == DataTypeKind::Void) {
            diagnostic_sink.report(DiagnosticCode::InvalidVariableType, "Cannot create a local variable of type 'void'", data_type_source_location);
            return nullptr;
        }

        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        return std::make_unique<VariableDefinitionStatement>(data_type, identifier_id, std::move(assignment_statement), data_type_source_location);
    }

}
