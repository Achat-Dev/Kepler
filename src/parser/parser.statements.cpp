// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    ASTNode* Parser::parse_statement() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        switch (current_token->token_kind) {
            case TokenKind::If:
                return parse_if();
            case TokenKind::For:
                return parse_for();
            case TokenKind::Return:
                return parse_return();
            case TokenKind::Identifier:
                return parse_identifier_statement();
            default:
                break;
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
        return nullptr;
    }

    ASTNode* Parser::parse_identifier_statement() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);
        switch (peek_next_token_kind(1, true)) {
            case TokenKind::Identifier:
                return parse_variable_definition();
            case TokenKind::Assignment: {
                KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
                const IdentifierPath identifier_path = identifier_id_to_path(std::get<StringId>(current_token->data), current_token->source_location);
                next_token(true); // eat identifier
                return parse_assignment(std::move(identifier_path));
            }
            case TokenKind::BracketOpen: {
                KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
                const IdentifierPath identifier_path = identifier_id_to_path(std::get<StringId>(current_token->data), current_token->source_location);
                next_token(true); // eat identifier
                return parse_call(std::nullopt, std::move(identifier_path));
            }
            case TokenKind::DoubleColon: {
                const auto module_path = parse_identifier_path(TokenKind::DoubleColon, IdentifierPathParseKind::ReturnAtLastIdentifier, "in module path");
                if (!module_path.has_value()) {
                    return nullptr;
                }
                const TokenKind next_token_kind = peek_next_token_kind(1, true);
                if (next_token_kind == TokenKind::BracketOpen) {
                    KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
                    const IdentifierPath identifier_path = identifier_id_to_path(std::get<StringId>(current_token->data), current_token->source_location);
                    next_token(true); // eat last identifier
                    return parse_call(std::move(module_path.value()), std::move(identifier_path));
                } else if (next_token_kind == TokenKind::Dot) {
                    const auto callee_identifier = parse_identifier_path(TokenKind::Dot, IdentifierPathParseKind::IncludeLastIdentifier, "in member path");
                    if (!callee_identifier.has_value()) {
                        return nullptr;
                    }
                    if (current_token->token_kind == TokenKind::BracketOpen) {
                        return parse_call(std::move(module_path.value()), std::move(callee_identifier.value()));
                    }
                }
                break;
            }
            case TokenKind::Dot: {
                const auto member_path_parse_result = parse_identifier_path(TokenKind::Dot, IdentifierPathParseKind::IncludeLastIdentifier, "in member path");
                if (!member_path_parse_result.has_value()) {
                    return nullptr;
                }
                if (current_token->token_kind == TokenKind::Assignment) {
                    return parse_assignment(std::move(member_path_parse_result.value()));
                } else if (current_token->token_kind == TokenKind::BracketOpen) {
                    return parse_call(std::nullopt, std::move(member_path_parse_result.value()));
                }
                break;
            }
            default:
                break;
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
        return nullptr;
    }

    AssignmentStatement* Parser::parse_assignment(IdentifierPath identifier_path) {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Assignment,
            "Required token: '{}', received: '{}'",
            TokenKind::Assignment,
            current_token->token_kind);
        KPL_ASSERT_THAT(!identifier_path.identifier_parts.empty());
        AssignmentTargetExpression assignment_target_expression;
        if (identifier_path.identifier_parts.size() == 1) {
            const IdentifierPathPart identifier_path_part = identifier_path.identifier_parts[0];
            assignment_target_expression = allocator->allocate<VariableExpression>(identifier_path_part.identifier_id, identifier_path_part.source_location);
        } else {
            assignment_target_expression = allocator->allocate<MemberExpression>(std::move(identifier_path));
        }

        const SourceLocation& assignment_source_location = current_token->source_location;
        next_token(true); // eat '='
        Expression* value_expression = parse_expression();
        if (!value_expression) {
            return nullptr;
        }

        return allocator->allocate<AssignmentStatement>(assignment_target_expression, value_expression, assignment_source_location);
    }

    IfStatement* Parser::parse_if() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::If || current_token->token_kind == TokenKind::Elseif,
            "Required token: '{}' or '{}', received: '{}'",
            TokenKind::If,
            TokenKind::Elseif,
            current_token->token_kind);
        const Token* if_token = current_token;

        Expression* condition = nullptr;
        next_token(true); // eat 'if' or 'elseif'
        if (current_token->token_kind != TokenKind::BracketOpen) {
            const std::string message = std::format("Expected '(' after '{}'", if_token->token_kind);
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            if (current_token->token_kind == TokenKind::End) {
                next_token(true); // eat 'end'
                return nullptr;
            }
        } else {
            next_token(true); // eat '('
            if (current_token->token_kind == TokenKind::BracketClose) {
                const std::string message = std::format("Missing '{}' condition, expected expression as condition", if_token->token_kind);
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, current_token->source_location);
                next_token(true);
            } else {
                // Parse 'if' condition
                condition = parse_expression();
                if (condition && current_token->token_kind != TokenKind::BracketClose) {
                    const std::string message = std::format("Expected ')' after condition in '{}'", if_token->token_kind);
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken, message, current_token->source_location);
                    recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                    if (current_token->token_kind == TokenKind::End) {
                        next_token(true); // eat 'end'
                        return nullptr;
                    }
                }
            }
        }

        if (condition && current_token->token_kind == TokenKind::BracketClose) {
            next_token(true); // eat ')'
        }
        const std::string message = std::format("'{}' statement was not closed with an 'end' keyword", if_token->token_kind);
        const auto if_body = parse_body<TokenKind::Elseif, TokenKind::Else, TokenKind::End>(message, if_token->source_location);

        // Setup behaviour for different 'if' ending cases
        switch (current_token->token_kind) {
            case TokenKind::Elseif: {
                // Don' eat the 'elseif', it will be eaten by parse_if
                IfStatement* elseif = parse_if();
                if (!condition || !if_body.has_value() || !elseif) {
                    return nullptr;
                }
                return allocator->allocate<IfStatement>(condition, std::move(if_body.value()), std::vector<ASTNode*>{elseif}, if_token->source_location);
            }
            case TokenKind::Else: {
                const SourceLocation& source_location = current_token->source_location;
                next_token(true); // eat 'else'
                const auto else_body = parse_body<TokenKind::End>("'else' statement was not closed with an 'end' keyword", source_location);
                next_token(true); // eat 'end'
                if (!condition || !if_body.has_value() || !else_body.has_value()) {
                    return nullptr;
                }
                return allocator->allocate<IfStatement>(condition, std::move(if_body.value()), std::move(else_body.value()), if_token->source_location);
            }
            case TokenKind::End: {
                next_token(true); // eat 'end'
                if (!condition || !if_body) {
                    return nullptr;
                }
                return allocator->allocate<IfStatement>(condition,
                    std::move(if_body.value()),
                    std::vector<ASTNode*>{},
                    if_token->source_location);
            }
            default:
                KPL_ASSERT_UNREACHABLE("Parsing an if statement failed");
        }
    }

    ForStatement* Parser::parse_for() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::For, "Required token: '{}', received: '{}'", TokenKind::For, current_token->token_kind);
        const SourceLocation& for_source_location = current_token->source_location;
        next_token(true); // eat 'for'
        if (current_token->token_kind != TokenKind::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->token_kind == TokenKind::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Missing 'for' definition, expected at least '(<type> <identifier> : <end_value>)'",
                current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        } else if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type after '(' in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        const Token* variable_type_token = current_token;
        next_token(true); // eat type
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after type in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        const Token* variable_identifier_token = current_token;
        next_token(true); // eat identifier
        if (current_token->token_kind != TokenKind::Colon) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ':' after identifier in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ':'
        Expression* first_value = parse_expression();
        if (!first_value) {
            if (current_token->token_kind == TokenKind::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenKind::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only end value is given, start and step are implicit
        if (current_token->token_kind == TokenKind::BracketClose) {
            const StringId integer_value_id = StringPool::get().store("0");
            IntegerLiteralExpression* start_value = allocator->allocate<IntegerLiteralExpression>(integer_value_id, variable_type_token->source_location);
            return create_for_statement(variable_type_token,
                variable_identifier_token,
                start_value,
                first_value,
                nullptr,
                for_source_location);
        } else if (current_token->token_kind != TokenKind::Comma) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after first expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        Expression* end_value = parse_expression();
        if (!end_value) {
            if (current_token->token_kind == TokenKind::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenKind::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Only start and end value are given, step is implicit
        if (current_token->token_kind == TokenKind::BracketClose) {
            return create_for_statement(variable_type_token,
                variable_identifier_token,
                first_value,
                end_value,
                nullptr,
                for_source_location);
        } else if (current_token->token_kind != TokenKind::Comma) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after second expression in 'for'", current_token->source_location);
            recover_for_definition_and_parse_body(for_source_location);
            return nullptr;
        }

        next_token(true); // eat ','
        Expression* step_value = parse_expression();
        if (!step_value) {
            if (current_token->token_kind == TokenKind::End) {
                next_token(true); // eat 'end'
            } else {
                // Parse the for body to collect diagnostics for it
                parse_body<TokenKind::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
            }
            return nullptr;
        }

        // Start, stop and end values are given
        if (current_token->token_kind == TokenKind::BracketClose) {
            return create_for_statement(variable_type_token,
                variable_identifier_token,
                first_value,
                end_value,
                step_value,
                for_source_location);
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' after third expression in 'for'", current_token->source_location);
        recover_for_definition_and_parse_body(for_source_location);
        return nullptr;
    }

    // clang-format off
    ForStatement* Parser::create_for_statement(const Token* variable_type_token,
        const Token* variable_identifier_token,
        Expression* start_value,
        Expression* end_value,
        Expression* step_value,
        SourceLocation for_source_location)
    {
        // clang-format on
        KPL_ASSERT_NOT_NULLPTR(variable_type_token);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(variable_type_token->data));
        KPL_ASSERT_NOT_NULLPTR(variable_identifier_token);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(variable_identifier_token->data));
        KPL_ASSERT_NOT_NULLPTR(start_value);
        KPL_ASSERT_NOT_NULLPTR(end_value);
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::BracketClose,
            "Required token: '{}', received: '{}'",
            TokenKind::BracketOpen,
            current_token->token_kind);
        next_token(true); // eat ')'
        const auto body = parse_body<TokenKind::End>("'for' statement was not closed with an 'end' keyword", for_source_location);
        next_token(true); // eat 'end'
        if (!body.has_value()) {
            return nullptr;
        }

        const StringId variable_type_id = std::get<StringId>(variable_type_token->data);
        const StringId variable_identifier_id = std::get<StringId>(variable_identifier_token->data);
        VariableExpression* variable = allocator->allocate<VariableExpression>(variable_identifier_id, variable_identifier_token->source_location);
        AssignmentStatement* assignment_statement = allocator->allocate<AssignmentStatement>(variable,
            start_value,
            variable_identifier_token->source_location);
        VariableDefinitionStatement* variable_definition_statement = allocator->allocate<VariableDefinitionStatement>(variable_type_id,
            variable_identifier_id,
            assignment_statement,
            variable_type_token->source_location);

        return allocator->allocate<ForStatement>(variable_definition_statement, end_value, step_value, std::move(body.value()), for_source_location);
    }

    void Parser::recover_for_definition_and_parse_body(SourceLocation source_location) {
        recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
        if (current_token->token_kind == TokenKind::End) {
            next_token(true); // eat 'end'
        } else {
            // Parse the for body to collect diagnostics for it (this also handles EOF)
            parse_body<TokenKind::End>("'for' statement was not closed with an 'end' keyword", source_location);
        }
    }

    ReturnStatement* Parser::parse_return() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Return,
            "Required token: '{}', received: '{}'",
            TokenKind::Return,
            current_token->token_kind);
        const SourceLocation& return_source_location = current_token->source_location;
        next_token(true); // eat 'return' keyword
        // This is not the most beautiful code, but it allows for better diagnostics
        switch (current_token->token_kind) {
            case TokenKind::Return:
            case TokenKind::If:
            case TokenKind::Else:
            case TokenKind::Elseif:
            case TokenKind::For: {
                const std::string message = std::format("Unexpected token '{}' after '{}' keyword. Must return an expression.",
                    current_token->token_kind,
                    TokenKind::Return);
                diagnostic_sink.report(DiagnosticCode::InvalidReturn, message, current_token->source_location);
                recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
            }
            case TokenKind::End:
                return allocator->allocate<ReturnStatement>(nullptr, return_source_location);
            default: {
                Expression* expression = parse_expression();
                if (!expression) {
                    return nullptr;
                }
                return allocator->allocate<ReturnStatement>(expression, return_source_location);
            }
        }
    }

    VariableDefinitionStatement* Parser::parse_variable_definition() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);
        const SourceLocation& type_source_location = current_token->source_location;
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId type_id = std::get<StringId>(current_token->data);
        next_token(true); // eat type identifier
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Expected identifier (types can only be used for variable definitions here)",
                current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        const Token* identifier_token = current_token;
        next_token(true); // eat identifier
        if (current_token->token_kind != TokenKind::Assignment) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Expected '=' after identifier for variable definition", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(identifier_token->data));
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        const IdentifierPath identifier_path = identifier_id_to_path(identifier_id, identifier_token->source_location);
        AssignmentStatement* assignment_statement = parse_assignment(std::move(identifier_path));
        if (!assignment_statement) {
            return nullptr; // parse_assignment already recovered, so no need to recover here
        }
        return allocator->allocate<VariableDefinitionStatement>(type_id, identifier_id, assignment_statement, type_source_location);
    }

}
