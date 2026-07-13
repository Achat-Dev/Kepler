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
#include "ast/expressions/literals/integer_literal_expression.hpp"
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
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kepler::parser {

    std::shared_ptr<ast::ASTNode> Parser::parse_statement() {
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
                const std::string& identifier = std::get<std::string>(current_token->data);
                const bool is_undefined_symbol = !semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier);
                const diagnostics::SourceLocation identifier_source_location = current_token->source_location;

                next_token(true); // eat identifier
                if (current_token->type == lexer::TokenType::Assignment) {
                    if (is_undefined_symbol) {
                        const std::string message = std::format("Undefined symbol '{}'", identifier);
                        diagnostic_sink.report(diagnostics::DiagnosticCode::UndefinedSymbol, message, file_path, identifier_source_location);
                        // Symbol doesn't exist, but act like it exists for further diagnostics, so no need to recover and/or return
                    }
                    return parse_assignment(identifier);
                } else if (current_token->type == lexer::TokenType::BracketOpen) {
                    if (is_undefined_symbol) {
                        const std::string message = std::format("Undefined symbol '{}'", identifier);
                        diagnostic_sink.report(diagnostics::DiagnosticCode::UndefinedSymbol, message, file_path, identifier_source_location);
                        // Symbol doesn't exist, but act like it exists for further diagnostics, so no need to recover and/or return
                    }
                    return parse_call(identifier);
                }

                previous_token(true); // Previous token for the diagnostic
                break;
            }

            default:
                break;
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), file_path, current_token->source_location);
        recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        return nullptr;
    }

    std::shared_ptr<ast::AssignmentStatement> Parser::parse_assignment(const std::string& identifier) {
        if (current_token->type != lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '=' after identifier in assignment", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '='
        const std::shared_ptr<ast::Expression> value_expression = parse_expression();
        if (!value_expression) {
            return nullptr; // parse_expression alredy recovered, so no need to recover here
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(identifier);
        return std::make_shared<ast::AssignmentStatement>(variable, value_expression);
    }

    std::shared_ptr<ast::IfStatement> Parser::parse_if() {
        const lexer::Token* if_token = current_token;

        std::shared_ptr<ast::Expression> condition = nullptr;
        next_token(true); // eat 'if' or 'elseif'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            const std::string message = std::format("Expected '(' after '{}'", if_token->type);
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            if (current_token->type == lexer::TokenType::End) {
                next_token(true); // eat 'end'
                return nullptr;
            }
        } else {
            next_token(true); // eat '('
            if (current_token->type == lexer::TokenType::BracketClose) {
                const std::string message = std::format("Missing '{}' condition, expected expression as condition", if_token->type);
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, file_path, current_token->source_location);
                next_token(true);
            } else {
                // Parse 'if' condition
                condition = parse_expression();
                if (condition && current_token->type != lexer::TokenType::BracketClose) {
                    const diagnostics::SourceLocation source_location(current_token->source_location.position + 1, 1);
                    const std::string message = std::format("Expected ')' after condition in '{}'", if_token->type);
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, file_path, source_location);
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
        const auto if_body = parse_body<lexer::TokenType::Elseif, lexer::TokenType::Else, lexer::TokenType::End>(message, if_token->source_location);

        // Setup behaviour for different 'if' ending cases
        switch (current_token->type) {
            case lexer::TokenType::Elseif: {
                // Don' eat the 'elseif', it will be eaten by parse_if
                const std::shared_ptr<ast::IfStatement> elseif = parse_if();
                if (!condition || !if_body || !elseif) {
                    return nullptr;
                }
                return std::make_shared<ast::IfStatement>(condition, std::move(*if_body), std::vector<std::shared_ptr<ast::ASTNode>>{elseif});
            }
            case lexer::TokenType::Else: {
                const diagnostics::SourceLocation source_location = current_token->source_location;
                next_token(true); // eat 'else'
                const auto else_body = parse_body<lexer::TokenType::End>("'else' statement was not closed with an 'end' keyword", source_location);
                next_token(true); // eat 'end'
                if (!condition || !if_body || !else_body) {
                    return nullptr;
                }
                return std::make_shared<ast::IfStatement>(condition, std::move(*if_body), std::move(*else_body));
            }
            case lexer::TokenType::End: {
                next_token(true); // eat 'end'
                if (!condition || !if_body) {
                    return nullptr;
                }
                return std::make_shared<ast::IfStatement>(condition, std::move(*if_body), std::vector<std::shared_ptr<ast::ASTNode>>{});
            }
            default:
                emergency_exit("I may have messed up parsing an if statement so badly that it neither ends with 'elseif', 'else' or 'end' nor have I encountered EOF. Whoops (ᵕ—ᗜ—)");
        }
    }

    std::shared_ptr<ast::ForStatement> Parser::parse_for() {
        const diagnostics::SourceLocation for_source_location = current_token->source_location;
        next_token(true); // eat 'for'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type == lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Missing 'for' definition, expected at least '(<datatype> <identifier> : <end_value>)'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        } else if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after '(' in 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }
        const lexer::Token* data_type_token = current_token;

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type in 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        const std::string& identifier = std::get<std::string>(current_token->data);
        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::Colon) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ':' after identifier in 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ':'
        const std::shared_ptr<ast::Expression> first_value = parse_expression();
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
            return create_for_statement(identifier, data_type_token, std::make_shared<ast::IntegerLiteralExpression>(0), first_value, nullptr, for_source_location);
        } else if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after first expression in 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        const std::shared_ptr<ast::Expression> end_value = parse_expression();
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
            return create_for_statement(identifier, data_type_token, first_value, end_value, nullptr, for_source_location);
        } else if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after second expression in 'for'", file_path, current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        const std::shared_ptr<ast::Expression> step_value = parse_expression();
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
            return create_for_statement(identifier, data_type_token, first_value, end_value, step_value, for_source_location);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after third expression in 'for'", file_path, current_token->source_location);
        recover_for_definition_and_parse_body(for_source_location);
        return nullptr;
    }

    std::shared_ptr<ast::ForStatement> Parser::create_for_statement(const std::string& variable_identifier, const lexer::Token* variable_data_type_token, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value, const diagnostics::SourceLocation& for_source_location) {
        next_token(true); // eat ')'
        const auto body = parse_body<lexer::TokenType::End>("'for' statement was not closed with an 'end' keyword", for_source_location);

        next_token(true); // eat 'end'
        if (!body) {
            return nullptr;
        }

        const type_system::DataTypeKind variable_data_type = std::get<type_system::DataTypeKind>(variable_data_type_token->data);
        const auto symbol_id = semantic_analysis::SymbolTable::get().create_variable(variable_identifier, variable_data_type);
        if (!symbol_id) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, symbol_id.error(), file_path, variable_data_type_token->source_location);
            return nullptr;
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(variable_identifier);
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = std::make_shared<ast::AssignmentStatement>(variable, start_value);
        const std::shared_ptr<ast::VariableDefinitionStatement> variable_definition_statement = std::make_shared<ast::VariableDefinitionStatement>(variable_data_type, variable_identifier, assignment_statement);

        return std::make_shared<ast::ForStatement>(variable_definition_statement, end_value, step_value, std::move(*body));
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

    std::shared_ptr<ast::ReturnStatement> Parser::parse_return() {
        next_token(true); // eat 'return' keyword
        if (current_parsing_function_return_type == type_system::DataTypeKind::None) {
            emergency_exit("Someone - and I am not going to say who (maybe because it was myself) - forgot to store the return type of the currently parsed function. And maybe, but just maybe, that's the reason why I'm currently shitting myself trying to parse a return statement without knowing the return type of the function.");
        }

        if (current_parsing_function_return_type == type_system::DataTypeKind::Void) {
            return std::make_shared<ast::ReturnStatement>(nullptr);
        }

        switch (current_token->type) {
            case lexer::TokenType::If:
            case lexer::TokenType::For:
            case lexer::TokenType::Return: {
                const std::string message = std::format("Cannot return '{}' statement", current_token->type);
                diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidReturnExpression, message, file_path, current_token->source_location);
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
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}' (data types can only be used for casting here)", *current_token), file_path, current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return nullptr;
            case lexer::TokenType::End:
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected expression after 'return' (a non-void function needs a return value)", file_path, current_token->source_location);
                return nullptr;

            default: break;
        }

        const std::shared_ptr<ast::Expression> expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression already recovered, so no need to recover here
        }

        return std::make_shared<ast::ReturnStatement>(expression);
    }

    std::shared_ptr<ast::VariableDefinitionStatement> Parser::parse_variable_definition() {
        const lexer::Token* data_type_token = current_token;
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier (data types can only be used for variable definitions here)", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }
        const std::string& identifier = std::get<std::string>(current_token->data);

        next_token(true); // eat identifier
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = parse_assignment(identifier);
        if (!assignment_statement) {
            return nullptr; // parse_assignment already recovered, so no need to recover here
        }

        if (data_type == type_system::DataTypeKind::Void) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidVariableType, "Cannot create a local variable of type 'void'", file_path, data_type_token->source_location);
            return nullptr;
        }

        const auto variable_id = semantic_analysis::SymbolTable::get().create_variable(identifier, data_type);
        if (!variable_id) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, variable_id.error(), file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        return std::make_shared<ast::VariableDefinitionStatement>(data_type, identifier, assignment_statement);
    }

}
