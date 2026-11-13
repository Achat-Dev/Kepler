// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
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
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_kind.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    Expression* Parser::parse_expression() {
        Expression* expression = parse_primary();
        if (!expression) {
            return nullptr;
        }

        if (current_token->token_kind == TokenKind::Operator) {
            return parse_binary_expression_rhs(expression, 0);
        }
        return expression;
    }

    Expression* Parser::parse_binary_expression_rhs(Expression* lhs, int expression_precedence) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        while (true) {
            if (current_token->token_kind != TokenKind::Operator) {
                return lhs;
            }

            const SourceLocation& operator_source_location = current_token->source_location;
            KPL_ASSERT_THAT(std::holds_alternative<OperatorKind>(current_token->data));
            const OperatorKind current_operator_kind = std::get<OperatorKind>(current_token->data);
            const int current_operator_precedence = get_operator_precedence(current_operator_kind);
            if (current_operator_precedence < expression_precedence) {
                return lhs;
            }

            next_token(true); // eat binary operator
            Expression* rhs = parse_primary();
            if (!rhs) {
                return nullptr;
            }

            if (current_token->token_kind != TokenKind::Operator) {
                return allocator->allocate<BinaryExpression>(current_operator_kind, lhs, rhs, operator_source_location);
            }

            KPL_ASSERT_THAT(std::holds_alternative<OperatorKind>(current_token->data));
            const OperatorKind next_operator_kind = std::get<OperatorKind>(current_token->data);
            const int next_operator_precedence = get_operator_precedence(next_operator_kind);
            if (current_operator_precedence < next_operator_precedence) {
                rhs = parse_binary_expression_rhs(rhs, current_operator_precedence + 1);
                if (!rhs) {
                    return nullptr;
                }
            }

            lhs = allocator->allocate<BinaryExpression>(current_operator_kind, lhs, rhs, operator_source_location);
        }
    }

    Expression* Parser::parse_primary() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        switch (current_token->token_kind) {
            case TokenKind::Identifier:
                return parse_identifier_expression();
            case TokenKind::Operator:
                KPL_ASSERT_THAT(std::holds_alternative<OperatorKind>(current_token->data));
                if (std::get<OperatorKind>(current_token->data) == OperatorKind::Minus) {
                    return parse_negative();
                }
                break;
            case TokenKind::Literal:
                return parse_literal();
            case TokenKind::BracketOpen:
                return parse_parenthesis();
            case TokenKind::CurlyBracketOpen:
                return parse_object_initializer();
            default:
                break;
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
        return nullptr;
    }

    Expression* Parser::parse_identifier_expression() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);

        switch (peek_next_token_kind(1, true)) {
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
                    next_token(true); // eat last identifer
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
                const auto member_path = parse_identifier_path(TokenKind::Dot, IdentifierPathParseKind::IncludeLastIdentifier, "in member path");
                if (!member_path.has_value()) {
                    return nullptr;
                }
                if (current_token->token_kind == TokenKind::BracketOpen) {
                    return parse_call(std::nullopt, std::move(member_path.value()));
                }
                return allocator->allocate<MemberExpression>(std::move(member_path.value()));
            }
            case TokenKind::CurlyBracketOpen:
                return parse_object_initializer();
            default: {
                KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
                const StringId identifier_id = std::get<StringId>(current_token->data);
                const SourceLocation& identifier_source_location = current_token->source_location;
                next_token(true); // eat identifier
                return allocator->allocate<VariableExpression>(identifier_id, identifier_source_location);
            }
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
        return nullptr;
    }

    CallExpression* Parser::parse_call(std::optional<IdentifierPath> module_path, IdentifierPath callee_identifier) {
        KPL_ASSERT_THAT(!callee_identifier.identifier_parts.empty());
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::BracketOpen,
            "Required token: '{}', received: '{}'",
            TokenKind::BracketOpen,
            current_token->token_kind);
        if (!module_path.has_value()) {
            module_path = IdentifierPath{};
        }

        if (current_token->token_kind != TokenKind::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after identifier for function call", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }
        next_token(true); // eat '('
        // Call with no arguments
        const SourceLocation& identifier_source_location = callee_identifier.source_location;
        if (current_token->token_kind == TokenKind::BracketClose) {
            next_token(true); // eat ')'
            return allocator->allocate<CallExpression>(
                std::move(callee_identifier),
                std::vector<Expression*>{},
                std::move(module_path.value()),
                identifier_source_location);
        }

        // Call with arguments
        std::vector<Expression*> arguments;
        while (current_token->token_kind != TokenKind::BracketClose) {
            Expression* argument = parse_expression();
            if (!argument) {
                return nullptr;
            }
            arguments.push_back(argument);

            if (current_token->token_kind == TokenKind::Comma) {
                next_token(true); // eat ','
                continue;
            }

            if (current_token->token_kind != TokenKind::BracketClose) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' or ',' in function argument list", current_token->source_location);
                recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return allocator->allocate<CallExpression>(
            std::move(callee_identifier),
            std::move(arguments),
            std::move(module_path.value()),
            identifier_source_location);
    }

    Expression* Parser::parse_literal() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Literal,
            "Required token: '{}', received: '{}'",
            TokenKind::Literal,
            current_token->token_kind);
        const SourceLocation& source_location = current_token->source_location;
        const auto literal_data = current_token->data;
        next_token(true); // eat the literal

        if (std::holds_alternative<double>(literal_data)) {
            return allocator->allocate<FloatingPointLiteralExpression>(std::get<double>(literal_data), source_location);
        } else if (std::holds_alternative<IntegerLiteralTokenData>(literal_data)) {
            return allocator->allocate<IntegerLiteralExpression>(std::get<IntegerLiteralTokenData>(literal_data).literal_id, source_location);
        } else if (std::holds_alternative<StringId>(literal_data)) {
            return allocator->allocate<StringLiteralExpression>(std::get<StringId>(literal_data), source_location);
        } else if (std::holds_alternative<bool>(literal_data)) {
            return allocator->allocate<BooleanLiteralExpression>(std::get<bool>(literal_data), source_location);
        }
        KPL_ASSERT_UNREACHABLE("Literal token doesn't contain the correct literal data");
    }

    Expression* Parser::parse_parenthesis() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::BracketOpen,
            "Required token: '{}', received: '{}'",
            TokenKind::BracketOpen,
            current_token->token_kind);
        next_token(true); // eat '('
        Expression* expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression has already recovered at this point, so no need to recover again
        }

        if (current_token->token_kind != TokenKind::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')'", current_token->source_location);
            recover(SynchronizationSet<TokenKind::BracketClose, TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<>{});
            if (current_token->token_kind == TokenKind::Newline) {
                next_token(true); // eat newline
                return nullptr;
            }
            if (current_token->token_kind == TokenKind::End) {
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return expression;
    }

    MathematicalNegationExpression* Parser::parse_negative() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Operator,
            "Required token: '{}', received: '{}'",
            TokenKind::Operator,
            current_token->token_kind);
        KPL_ASSERT_THAT(std::holds_alternative<OperatorKind>(current_token->data));
        const OperatorKind operator_kind = std::get<OperatorKind>(current_token->data);
        KPL_ASSERT_THAT(operator_kind == OperatorKind::Minus, "Required operator: '{}', received: '{}'", OperatorKind::Minus, operator_kind);
        const SourceLocation& source_location = current_token->source_location;
        next_token(true); // eat '-'
        Expression* expression = nullptr;
        switch (current_token->token_kind) {
            case TokenKind::Identifier:
                expression = parse_identifier_expression();
                break;
            case TokenKind::BracketOpen:
                expression = parse_parenthesis();
                break;
            case TokenKind::Literal:
                // If the literal cannot be mathematically negated, fall through to the default case
                if (std::holds_alternative<double>(current_token->data) || std::holds_alternative<IntegerLiteralTokenData>(current_token->data)) {
                    expression = parse_literal();
                    // Adjust the source location of the literal to include the minus sign
                    const uint32_t source_location_size = (expression->source_location.position + expression->source_location.size) - source_location.position;
                    expression->source_location = {
                        .file_id = expression->source_location.file_id,
                        .position = source_location.position,
                        .size = source_location_size,
                    };
                    break;
                }
            default:
                const std::string message = std::format("Mathematical negation of '{}' is not supported", *current_token);
                diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, source_location);
                recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
        }

        if (!expression) {
            return nullptr;
        }
        return allocator->allocate<MathematicalNegationExpression>(expression, source_location);
    }

    ObjectInitializerExpression* Parser::parse_object_initializer() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        StringId type_identifier_id;
        SourceLocation type_source_location;
        if (current_token->token_kind == TokenKind::Identifier) {
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            type_identifier_id = std::get<StringId>(current_token->data);
            type_source_location = current_token->source_location;
            next_token(true); // eat type identifier
        }
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::CurlyBracketOpen,
            "Required token: '{}', received: '{}'",
            TokenKind::CurlyBracketOpen,
            current_token->token_kind);
        const SourceLocation& initializer_source_location = current_token->source_location;
        next_token(true); // eat '{'
        // The same check is done later on at the start of the while loop, but do it here again for a better diagnostic
        if (current_token->token_kind != TokenKind::Identifier && current_token->token_kind != TokenKind::CurlyBracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                "Expected identifier or '}' after '{' in object initializer",
                current_token->source_location);
            recover(SynchronizationSet<TokenKind::CurlyBracketClose>{}, SynchronizationSet<TokenKind::CurlyBracketClose>{});
            return nullptr;
        }

        std::vector<MemberInitializerData> member_initializers;
        while (current_token->token_kind != TokenKind::CurlyBracketClose) {
            if (current_token->token_kind != TokenKind::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                    "Expected identifier or '}' after ',' in object initializer",
                    current_token->source_location);
                recover(SynchronizationSet<TokenKind::CurlyBracketClose>{}, SynchronizationSet<TokenKind::CurlyBracketClose>{});
                return nullptr;
            }
            const Token* member_identifier_token = current_token;
            next_token(true); // eat identifier

            if (current_token->token_kind != TokenKind::Colon) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                    "Expected ':' after identifier in object initializer",
                    current_token->source_location);
                recover(SynchronizationSet<TokenKind::CurlyBracketClose>{}, SynchronizationSet<TokenKind::CurlyBracketClose>{});
                return nullptr;
            }
            next_token(true); // eat ':'
            // Check for a closing curly bracket for a better diagnostic
            if (current_token->token_kind == TokenKind::CurlyBracketClose) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                    "Expected value after ':' in object initializer",
                    current_token->source_location);
                next_token(true); // eat '}'
                return nullptr;
            }

            Expression* member_value_expression = parse_expression();
            if (!member_value_expression) {
                return nullptr;
            }

            if (current_token->token_kind != TokenKind::Comma && current_token->token_kind != TokenKind::CurlyBracketClose) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                    "Expected ',' or '}' after value in object initializer",
                    current_token->source_location);
                recover(SynchronizationSet<TokenKind::CurlyBracketClose>{}, SynchronizationSet<TokenKind::CurlyBracketClose>{});
                return nullptr;
            }
            if (current_token->token_kind == TokenKind::Comma) {
                next_token(true); // eat ','
            }

            KPL_ASSERT_THAT(std::holds_alternative<StringId>(member_identifier_token->data));
            member_initializers.push_back({
                .identifier_id = std::get<StringId>(member_identifier_token->data),
                .source_location = member_identifier_token->source_location,
                .value_expression = member_value_expression,
            });
        }

        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::CurlyBracketClose);
        next_token(true); // eat '}'
        return allocator->allocate<ObjectInitializerExpression>(type_identifier_id,
            std::move(member_initializers),
            initializer_source_location,
            std::move(type_source_location));
    }

}
