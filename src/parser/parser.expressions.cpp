// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
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
#include "ast/expressions/negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include "emergency.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "semantic_analysis/string_table.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::parser {

    int Parser::get_operator_precedence(lexer::OperatorType operator_type) const {
        switch (operator_type) {
            case lexer::OperatorType::LessThan:
            case lexer::OperatorType::GreaterThan:
            case lexer::OperatorType::Equals:
            case lexer::OperatorType::NotEquals:
            case lexer::OperatorType::LessEquals:
            case lexer::OperatorType::GreaterEquals:
                return 10;

            case lexer::OperatorType::Plus:
            case lexer::OperatorType::Minus:
                return 20;

            case lexer::OperatorType::Multiplication:
            case lexer::OperatorType::Division:
                return 30;
            default:
                emergency_exit("Binary operator '{}' doesn't have a precedence associated to it\n{}If this was intended, what the fuck where you thinking past me?", operator_type, log::styling::last_indented);
        }
    }

    std::shared_ptr<ast::Expression> Parser::parse_expression() {
        const std::shared_ptr<ast::Expression> expression = parse_primary();
        if (!expression) {
            return nullptr;
        }

        if (current_token->type == lexer::TokenType::Operator) {
            return parse_binary_expression_rhs(expression, 0);
        }
        return expression;
    }

    std::shared_ptr<ast::Expression> Parser::parse_binary_expression_rhs(std::shared_ptr<ast::Expression> lhs_expression, int expression_precedence) {
        while (true) {
            if (current_token->type != lexer::TokenType::Operator) {
                return lhs_expression;
            }

            const lexer::OperatorType current_operator_type = std::get<lexer::OperatorType>(current_token->data);
            const int current_operator_precedence = get_operator_precedence(current_operator_type);
            if (current_operator_precedence < expression_precedence) {
                return lhs_expression;
            }

            next_token(true); // eat binary operator

            // Do a bit of lookahead for better diagnostics
            const size_t continuation_token_index = current_token_index;
            if (current_token->type == lexer::TokenType::DataType) {
                next_token(true);
                if (current_token->type != lexer::TokenType::BracketOpen) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' (data types can only be used for casting inside of a binary expression)", file_path, current_token->source_location);
                    recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                    return nullptr;
                }
                jump_to_token(continuation_token_index);
            }

            std::shared_ptr<ast::Expression> rhs_expression = parse_primary();
            if (!rhs_expression) {
                return nullptr;
            }

            if (current_token->type != lexer::TokenType::Operator) {
                return std::make_shared<ast::BinaryExpression>(current_operator_type, lhs_expression, rhs_expression);
            }

            const lexer::OperatorType next_operator_type = std::get<lexer::OperatorType>(current_token->data);
            const int next_operator_precedence = get_operator_precedence(next_operator_type);
            if (current_operator_precedence < next_operator_precedence) {
                rhs_expression = parse_binary_expression_rhs(rhs_expression, current_operator_precedence + 1);
                if (!rhs_expression) {
                    return nullptr;
                }
            }

            lhs_expression = std::make_shared<ast::BinaryExpression>(current_operator_type, lhs_expression, rhs_expression);
        }
    }

    std::shared_ptr<ast::Expression> Parser::parse_primary() {
        switch (current_token->type) {
            case lexer::TokenType::Identifier:
                return parse_identifier();
            case lexer::TokenType::DataType:
                return parse_cast();
            case lexer::TokenType::Operator:
                if (std::get<lexer::OperatorType>(current_token->data) == lexer::OperatorType::Minus) {
                    return parse_negative();
                }
                break;
            case lexer::TokenType::Literal:
                return parse_literal();
            case lexer::TokenType::BracketOpen:
                return parse_parenthesis();
            default: break;
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), file_path, current_token->source_location);
        recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        return nullptr;
    }

    std::shared_ptr<ast::Expression> Parser::parse_identifier() {
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        if (!semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier_id)) {
            const std::string message = std::format("Undefined symbol '{}'", semantic_analysis::StringTable::get().lookup(identifier_id));
            diagnostic_sink.report(diagnostics::DiagnosticCode::UndefinedSymbol, message, file_path, current_token->source_location);
            // Symbol doesn't exist, but act like it exists for further diagnostics, so no need to recover and/or return
        }

        next_token(true); // eat identifier
        if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_call(identifier_id);
        }
        return std::make_shared<ast::VariableExpression>(identifier_id);
    }

    std::shared_ptr<ast::CallExpression> Parser::parse_call(semantic_analysis::StringId identifier_id) {
        next_token(true); // eat '('

        // Call with no arguments
        if (current_token->type == lexer::TokenType::BracketClose) {
            next_token(true); // eat ')'
            return std::make_shared<ast::CallExpression>(identifier_id, std::vector<std::shared_ptr<ast::Expression>>{});
        }

        // Call with arguments
        std::vector<std::shared_ptr<ast::Expression>> args;
        while (current_token->type != lexer::TokenType::BracketClose) {
            const std::shared_ptr<ast::Expression> arg = parse_expression();
            if (!arg) {
                return nullptr;
            }
            args.push_back(arg);

            if (current_token->type == lexer::TokenType::Comma) {
                next_token(true); // eat ','
                continue;
            }

            if (current_token->type != lexer::TokenType::BracketClose) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' or ',' in function argument list", file_path, current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Comma,
                            lexer::TokenType::BracketClose,
                            lexer::TokenType::Newline,
                            lexer::TokenType::End>{},
                    SynchronizationSet<lexer::TokenType::Comma, lexer::TokenType::Newline>{});
                if (current_token->type == lexer::TokenType::End) {
                    return nullptr;
                }
            }
        }

        next_token(true); // eat ')'
        return std::make_shared<ast::CallExpression>(identifier_id, std::move(args));
    }

    std::shared_ptr<ast::Expression> Parser::parse_literal() {
        const auto literal_data = current_token->data;
        next_token(true); // eat the literal

        if (std::holds_alternative<double>(literal_data)) {
            return std::make_shared<ast::FloatingPointLiteralExpression>(std::get<double>(literal_data));
        } else if (std::holds_alternative<int64_t>(literal_data)) {
            return std::make_shared<ast::IntegerLiteralExpression>(std::get<int64_t>(literal_data));
        } else if (std::holds_alternative<semantic_analysis::StringId>(literal_data)) {
            return std::make_shared<ast::StringLiteralExpression>(std::get<semantic_analysis::StringId>(literal_data));
        } else if (std::holds_alternative<bool>(literal_data)) {
            return std::make_shared<ast::BooleanLiteralExpression>(std::get<bool>(literal_data));
        } else {
            emergency_exit("Literal token doesn't contain the literal data? Tokenization, wtf?");
        }
    }

    std::shared_ptr<ast::Expression> Parser::parse_parenthesis() {
        next_token(true); // eat '('
        const std::shared_ptr<ast::Expression> expression = parse_expression();
        if (!expression) {
            return nullptr; // parse_expression has already recovered at this point, so no need to recover again
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')'", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::BracketClose, lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<>{});
            if (current_token->type == lexer::TokenType::Newline) {
                next_token(true);
                return nullptr;
            }
            if (current_token->type == lexer::TokenType::End) {
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        return expression;
    }

    std::shared_ptr<ast::NegationExpression> Parser::parse_negative() {
        next_token(true); // eat '-'
        std::shared_ptr<ast::Expression> expression = nullptr;
        switch (current_token->type) {
            case lexer::TokenType::Identifier:
                // TODO: Check if the symbol behind the identifier has a type that can be negated
                expression = parse_identifier();
                break;
            case lexer::TokenType::BracketOpen:
                expression = parse_parenthesis();
                break;
            case lexer::TokenType::Literal:
                // If the literal cannot be mathematically negated, fall through to the default case
                if (std::holds_alternative<double>(current_token->data) || std::holds_alternative<int64_t>(current_token->data)) {
                    expression = parse_literal();
                    break;
                }
            default:
                const size_t recovery_token_index = current_token_index;
                previous_token(true);
                const std::string message = std::format("Mathematical negation of '{}' is not supported", *current_token);
                diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidMathematicalNegation, message, file_path, current_token->source_location);
                jump_to_token(recovery_token_index);
                recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return nullptr;
        }

        if (!expression) {
            return nullptr;
        }
        return std::make_shared<ast::NegationExpression>(expression);
    }

    std::shared_ptr<ast::CastExpression> Parser::parse_cast() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after data type for cast", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type == lexer::TokenType::BracketClose) {
            diagnostics::SourceLocation source_location{
                current_token->source_location.position - 1,
                2};
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidCast, "Empty cast", file_path, current_token->source_location);
            next_token(true); // eat ')'
            return nullptr;
        }

        const std::shared_ptr<ast::Expression> expression_to_cast = parse_expression();
        if (!expression_to_cast) {
            return nullptr;
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            const size_t recovery_token_index = current_token_index;
            previous_token(true);
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after expression in cast", file_path, current_token->source_location);
            jump_to_token(recovery_token_index);

            recover(SynchronizationSet<lexer::TokenType::BracketClose, lexer::TokenType::Newline, lexer::TokenType::End>{}, SynchronizationSet<>{});
            if (current_token->type == lexer::TokenType::Newline) {
                next_token(true);
                return nullptr;
            }
            if (current_token->type == lexer::TokenType::End) {
                return nullptr;
            }
        }

        next_token(true); // eat ')'
        if (data_type == type_system::DataTypeKind::Void) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidCast, "Cannot cast a value to 'void'", file_path, current_token->source_location);
            return nullptr;
        }
        return std::make_shared<ast::CastExpression>(expression_to_cast, data_type);
    }
}
