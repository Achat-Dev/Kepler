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
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    int Parser::get_operator_precedence(OperatorType operator_type) const {
        switch (operator_type) {
            case OperatorType::LessThan:
            case OperatorType::GreaterThan:
            case OperatorType::Equals:
            case OperatorType::NotEquals:
            case OperatorType::LessEquals:
            case OperatorType::GreaterEquals:
                return 10;

            case OperatorType::Plus:
            case OperatorType::Minus:
                return 20;

            case OperatorType::Multiplication:
            case OperatorType::Division:
                return 30;
        }

        KPL_ASSERT_UNREACHABLE("Missing binary operator precedence implementation for operator '{}'", static_cast<int>(operator_type));
    }

    std::unique_ptr<Expression> Parser::parse_expression() {
        std::unique_ptr<Expression> expression = parse_primary();
        if (!expression) {
            return nullptr;
        }

        if (current_token->type == TokenType::Operator) {
            return parse_binary_expression_rhs(std::move(expression), 0);
        }
        return expression;
    }

    std::unique_ptr<Expression> Parser::parse_binary_expression_rhs(std::unique_ptr<Expression> lhs, int expression_precedence) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        while (true) {
            if (current_token->type != TokenType::Operator) {
                return lhs;
            }

            const SourceLocation& operator_source_location = current_token->source_location;
            KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, OperatorType, "Binary expression operator token");
            const OperatorType current_operator_type = std::get<OperatorType>(current_token->data);
            const int current_operator_precedence = get_operator_precedence(current_operator_type);
            if (current_operator_precedence < expression_precedence) {
                return lhs;
            }

            next_token(true); // eat binary operator

            // Do a bit of lookahead for better diagnostics
            const size_t continuation_token_index = current_token_index;
            if (current_token->type == TokenType::Type) {
                next_token(true);
                if (current_token->type != TokenType::BracketOpen) {
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        "Expected '(' (types can only be used for casting inside of a binary expression)",
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                    return nullptr;
                }
                jump_to_token(continuation_token_index);
            }

            std::unique_ptr<Expression> rhs = parse_primary();
            if (!rhs) {
                return nullptr;
            }

            if (current_token->type != TokenType::Operator) {
                return std::make_unique<BinaryExpression>(current_operator_type,
                    std::move(lhs),
                    std::move(rhs),
                    operator_source_location);
            }

            KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, OperatorType, "Binary expression next operator token");
            const OperatorType next_operator_type = std::get<OperatorType>(current_token->data);
            const int next_operator_precedence = get_operator_precedence(next_operator_type);
            if (current_operator_precedence < next_operator_precedence) {
                rhs = parse_binary_expression_rhs(std::move(rhs), current_operator_precedence + 1);
                if (!rhs) {
                    return nullptr;
                }
            }

            lhs = std::make_unique<BinaryExpression>(current_operator_type,
                std::move(lhs),
                std::move(rhs),
                operator_source_location);
        }
    }

    std::unique_ptr<Expression> Parser::parse_primary() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        switch (current_token->type) {
            case TokenType::Identifier:
                return parse_identifier();
            case TokenType::Type:
                return parse_cast();
            case TokenType::Operator:
                KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, OperatorType, "Mathematical negation operator token");
                if (std::get<OperatorType>(current_token->data) == OperatorType::Minus) {
                    return parse_negative();
                }
                break;
            case TokenType::Literal:
                return parse_literal();
            case TokenType::BracketOpen:
                return parse_parenthesis();
            default:
                break;
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), current_token->source_location);
        recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<Expression> Parser::parse_identifier() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Identifier,
            "Parsing identifier requires '{}' token, received '{}'",
            TokenType::Identifier,
            current_token->type);
        const Token* identifier_token = current_token;
        next_token(true); // eat identifier
        if (current_token->type == TokenType::BracketOpen) {
            return parse_call(identifier_token);
        }

        KPL_ASSERT_HOLDS_ALTERNATIVE(identifier_token->data, StringId, "Identifier token");
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        return std::make_unique<VariableExpression>(identifier_id, identifier_token->source_location);
    }

    std::unique_ptr<CallExpression> Parser::parse_call(const Token* identifier_token) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::BracketOpen,
            "Parsing call expression requires '{}' token, received '{}'",
            TokenType::BracketOpen,
            current_token->type);
        KPL_ASSERT_NOT_NULLPTR(identifier_token);
        KPL_ASSERT_HOLDS_ALTERNATIVE(identifier_token->data, StringId, "Call identifier token");
        const StringId identifier_id = std::get<StringId>(identifier_token->data);

        next_token(true); // eat '('
        // Call with no arguments
        if (current_token->type == TokenType::BracketClose) {
            next_token(true); // eat ')'
            return std::make_unique<CallExpression>(identifier_id, std::vector<std::unique_ptr<Expression>>{}, identifier_token->source_location);
        }

        // Call with arguments
        std::vector<std::unique_ptr<Expression>> args;
        while (current_token->type != TokenType::BracketClose) {
            std::unique_ptr<Expression> arg = parse_expression();
            if (!arg) {
                return nullptr;
            }
            args.push_back(std::move(arg));

            if (current_token->type == TokenType::Comma) {
                next_token(true); // eat ','
                continue;
            }

            if (current_token->type != TokenType::BracketClose) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' or ',' in function argument list", current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return std::make_unique<CallExpression>(identifier_id, std::move(args), identifier_token->source_location);
    }

    std::unique_ptr<Expression> Parser::parse_literal() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Literal,
            "Parsing literal requires '{}' token, received '{}'",
            TokenType::Literal,
            current_token->type);
        const SourceLocation& source_location = current_token->source_location;
        const auto literal_data = current_token->data;
        next_token(true); // eat the literal

        if (std::holds_alternative<double>(literal_data)) {
            return std::make_unique<FloatingPointLiteralExpression>(std::get<double>(literal_data), source_location);
        } else if (std::holds_alternative<int64_t>(literal_data)) {
            return std::make_unique<IntegerLiteralExpression>(std::get<int64_t>(literal_data), source_location);
        } else if (std::holds_alternative<StringId>(literal_data)) {
            return std::make_unique<StringLiteralExpression>(std::get<StringId>(literal_data), source_location);
        } else if (std::holds_alternative<bool>(literal_data)) {
            return std::make_unique<BooleanLiteralExpression>(std::get<bool>(literal_data), source_location);
        }

        KPL_ASSERT_UNREACHABLE("Literal token doesn't contain the correct literal data");
    }

    std::unique_ptr<Expression> Parser::parse_parenthesis() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::BracketOpen,
            "Parsing parenthesis requires '{}' token, received '{}'",
            TokenType::BracketOpen,
            current_token->type);
        next_token(true); // eat '('
        std::unique_ptr<Expression> expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression has already recovered at this point, so no need to recover again
        }

        if (current_token->type != TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')'", current_token->source_location);
            recover(SynchronizationSet<TokenType::BracketClose, TokenType::Newline, TokenType::End>{}, SynchronizationSet<>{});
            if (current_token->type == TokenType::Newline) {
                next_token(true); // eat newline
                return nullptr;
            }
            if (current_token->type == TokenType::End) {
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return expression;
    }

    std::unique_ptr<MathematicalNegationExpression> Parser::parse_negative() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Operator,
            "Parsing mathematical negation requires '{}' token, received '{}'",
            TokenType::Operator,
            current_token->type);
        KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, OperatorType, "Mathematical negation operator token");
        const OperatorType operator_type = std::get<OperatorType>(current_token->data);
        KPL_ASSERT_THAT(operator_type == OperatorType::Minus,
            "Parsing mathematical negation requires operator '{}', received operator '{}'",
            OperatorType::Minus,
            operator_type);
        const SourceLocation& source_location = current_token->source_location;
        next_token(true); // eat '-'
        std::unique_ptr<Expression> expression = nullptr;
        switch (current_token->type) {
            case TokenType::Identifier:
                expression = parse_identifier();
                break;
            case TokenType::BracketOpen:
                expression = parse_parenthesis();
                break;
            case TokenType::Literal:
                // If the literal cannot be mathematically negated, fall through to the default case
                if (std::holds_alternative<double>(current_token->data) || std::holds_alternative<int64_t>(current_token->data)) {
                    expression = parse_literal();
                    break;
                }
            default:
                const size_t recovery_token_index = current_token_index;
                previous_token(true);
                const std::string message = std::format("Mathematical negation of '{}' is not supported", *current_token);
                diagnostic_sink.report(DiagnosticCode::InvalidMathematicalNegation, message, current_token->source_location);
                jump_to_token(recovery_token_index);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
        }

        if (!expression) {
            return nullptr;
        }
        return std::make_unique<MathematicalNegationExpression>(std::move(expression), source_location);
    }

    std::unique_ptr<CastExpression> Parser::parse_cast() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Type,
            "Parsing cast requires '{}' token, received '{}'",
            TokenType::Type,
            current_token->type);
        KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Cast type token");
        const StringId type_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type
        if (current_token->type != TokenType::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after type for cast", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type == TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::InvalidCast, "Empty cast", current_token->source_location);
            next_token(true); // eat ')'
            return nullptr;
        }

        std::unique_ptr<Expression> expression_to_cast = parse_expression();
        if (!expression_to_cast) {
            return nullptr;
        }

        if (current_token->type != TokenType::BracketClose) {
            const size_t recovery_token_index = current_token_index;
            previous_token(true);
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' after expression in cast", current_token->source_location);
            jump_to_token(recovery_token_index);

            recover(SynchronizationSet<TokenType::BracketClose, TokenType::Newline, TokenType::End>{}, SynchronizationSet<>{});
            if (current_token->type == TokenType::Newline) {
                next_token(true);
                return nullptr;
            }
            if (current_token->type == TokenType::End) {
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return std::make_unique<CastExpression>(type_id, std::move(expression_to_cast), type_source_location);
    }
}
