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
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "emergency.hpp"
#include "error_code.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "semantic_analysis/string_table.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::parser {

    std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, ErrorCode> Parser::parse() {
        if (tokens.back().type != lexer::TokenType::EndOfFile) {
            log(log_type::INTERNAL_ERROR, "Parser received token stream without EOF token, my tokenizer seems to have fucked up somewhere");
            emergency_exit();
        }

        std::vector<std::shared_ptr<ast::ASTNode>> result;
        while (true) {
            switch (current_token->type) {
                case lexer::TokenType::DataType: {
                    const auto ast_node = parse_top_level_data_type();
                    if (!ast_node) {
                        return std::unexpected(ast_node.error());
                    }
                    result.push_back(*ast_node);
                    break;
                }
                case lexer::TokenType::EndOfFile: return result;
                case lexer::TokenType::Extern: {
                    const auto ast_node = parse_extern();
                    if (!ast_node) {
                        return std::unexpected(ast_node.error());
                    }
                    result.push_back(*ast_node);
                    break;
                }
                default:
                    log(log_type::PARSING_ERROR, "Unexpected token of type '", current_token->type, "' on top level, expected 'extern' or data type");
                    return std::unexpected(ErrorCode::ParserUnexpectedToken);
            }
        }

        return result;
    }

    int Parser::get_operator_precedence(lexer::OperatorType operator_type) const {
        switch (operator_type) {
            case lexer::OperatorType::LessThan:
            case lexer::OperatorType::GreaterThan:
            case lexer::OperatorType::Equals:
            case lexer::OperatorType::NotEquals:
            case lexer::OperatorType::LessEquals:
            case lexer::OperatorType::GreaterEquals: return 10;

            case lexer::OperatorType::Plus:
            case lexer::OperatorType::Minus: return 20;

            case lexer::OperatorType::Multiplication:
            case lexer::OperatorType::Division: return 30;
            default:
                log(log_type::INTERNAL_ERROR, "Binary operator '", operator_type, "' doesn't have a precedence associated to it\n",
                    log_type::LAST_INDENTED, "If this was intended, what the fuck where you thinking past me?");
                emergency_exit();
                std::abort();
        }
    }

    void Parser::next_token() {
        if (current_token_index < tokens.size() - 1) {
            current_token_index++;
            current_token = &tokens[current_token_index];
        }
    }

    void Parser::previous_token() {
        if (current_token_index > 0) {
            current_token_index--;
            current_token = &tokens[current_token_index];
        }
    }

    std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> Parser::parse_extern() {
        log_verbose("Parsing an extern");
        next_token(); // eat 'extern' keyword

        if (current_token->type != lexer::TokenType::DataType) {
            log(log_type::PARSING_ERROR, "Expected data type after 'extern'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        semantic_analysis::SymbolTable::get().open_scope();
        const auto prototype_id = parse_prototype(ast::Prototype::LinkageType::External);
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }
        semantic_analysis::SymbolTable::get().close_scope();

        return std::make_shared<ast::Prototype>(*prototype_id);
    }

    std::expected<semantic_analysis::SymbolId, ErrorCode> Parser::parse_prototype(ast::Prototype::LinkageType linkage_type) {
        const type_system::DataTypeKind return_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            log(log_type::PARSING_ERROR, "[ Prototype ]: Expected identifier after return type of prototype");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(); // eat identifier
        if (current_token->type != lexer::TokenType::BracketOpen) {
            log(log_type::PARSING_ERROR, "[ Prototype ]: Expected '(' after prototype name");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '('
        if (current_token->type != lexer::TokenType::DataType && current_token->type != lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "[ Prototype ]: Expected data type or ')' after '(' in prototype");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        // Parse parameters
        semantic_analysis::PrototypeSymbolData prototype_symbol_data;
        prototype_symbol_data.linkage_type = linkage_type;
        while (current_token->type == lexer::TokenType::DataType) {
            const type_system::DataTypeKind parameter_type = std::get<type_system::DataTypeKind>(current_token->data);

            next_token(); // eat data type
            if (current_token->type != lexer::TokenType::Identifier) {
                log(log_type::PARSING_ERROR, "[ Prototype ]: Expected identifier after parameter data type");
                return std::unexpected(ErrorCode::ParserUnexpectedToken);
            }

            const semantic_analysis::StringId parameter_identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
            const auto parameter_id = semantic_analysis::SymbolTable::get().create_variable(parameter_identifier_id, parameter_type);
            if (!parameter_id) {
                return std::unexpected(parameter_id.error());
            }
            prototype_symbol_data.parameter_ids.push_back(*parameter_id);

            next_token(); // eat identifier
            if (current_token->type == lexer::TokenType::Comma) {
                next_token(); // eat ','

                if (current_token->type != lexer::TokenType::DataType) {
                    log(log_type::PARSING_ERROR, "[ Prototype ]: Expected data type after ',' in prototype parameters");
                    return std::unexpected(ErrorCode::ParserUnexpectedToken);
                }
            }
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "[ Prototype ]: Expected ')' after function parameters in prototype");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }
        next_token(); // eat ')'

        const auto prototype_id = semantic_analysis::SymbolTable::get().create_prototype(identifier_id, return_type, std::move(prototype_symbol_data));
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }

        return *prototype_id;
    }

    std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> Parser::parse_top_level_data_type() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            log(log_type::PARSING_ERROR, "Expected identifier after data type on top level");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        next_token(); // eat identifier

        // Variable definition
        if (current_token->type == lexer::TokenType::Assignment) {
            if (data_type == type_system::DataTypeKind::Void) {
                log(log_type::PARSING_ERROR, "Cannot create a global variable of type 'void'");
                return std::unexpected(ErrorCode::ParserInvalidVariableType);
            }

            log(log_type::UNSUPPORTED, "Global variables are not supported yet");
            return std::unexpected(ErrorCode::Unsupported);
        }
        // Function definition
        else if (current_token->type == lexer::TokenType::BracketOpen) {
            // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
            previous_token();
            previous_token();
            return parse_function(data_type, identifier_id);
        }

        log(log_type::PARSING_ERROR, "Expected either an assignment operator or a '(' after identifier on top level");
        return std::unexpected(ErrorCode::ParserUnexpectedToken);
    }

    std::expected<std::shared_ptr<ast::Function>, ErrorCode> Parser::parse_function(type_system::DataTypeKind return_type, semantic_analysis::StringId identifier_id) {
        semantic_analysis::SymbolTable::get().open_scope();

        const auto prototype_id = parse_prototype(ast::Prototype::LinkageType::Internal);
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }

        current_parsing_function_return_type = return_type;

        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                log(log_type::PARSING_ERROR, "Function was not closed with an 'end' keyword");
                return std::unexpected(ErrorCode::ParserMissingEndKeyword);
            }

            const auto statement = parse_statement();
            if (!statement) {
                // log(log_type::PARSING_ERROR, "Invalid expression in function body");
                return std::unexpected(statement.error());
            }
            body.push_back(*statement);
        }

        next_token(); // eat 'end'
        current_parsing_function_return_type = type_system::DataTypeKind::None;
        semantic_analysis::SymbolTable::get().close_scope();

        return std::make_shared<ast::Function>(*prototype_id, std::move(body));
    }

    std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> Parser::parse_statement() {
        switch (current_token->type) {
            case lexer::TokenType::If:
                return parse_if();
            case lexer::TokenType::For:
                return parse_for();
            case lexer::TokenType::Return:
                return parse_return();
            case lexer::TokenType::DataType: {
                next_token(); // eat data type
                if (current_token->type == lexer::TokenType::Identifier) {
                    previous_token();
                    return parse_variable_declaration();
                }
                break;
            }
            case lexer::TokenType::Identifier: {
                next_token(); // eat identifier
                const lexer::TokenType next_token_type = current_token->type;
                previous_token();
                if (next_token_type == lexer::TokenType::Assignment) {
                    return parse_assignment();
                } else if (next_token_type == lexer::TokenType::BracketOpen) {
                    return parse_identifier();
                }
                break;
            }

            default:
                break;
        }

        log(log_type::PARSING_ERROR, "Unexpected token '", *current_token, "'");
        return std::unexpected(ErrorCode::ParserUnexpectedToken);
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_expression() {
        const auto expression = parse_primary();
        if (!expression) {
            return std::unexpected(expression.error());
        }

        if (current_token->type == lexer::TokenType::Operator) {
            return parse_binary_expression_rhs(*expression, 0);
        }
        return expression;
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_binary_expression_rhs(std::shared_ptr<ast::Expression> lhs_expression, int expression_precedence) {
        while (true) {
            if (current_token->type != lexer::TokenType::Operator) {
                return lhs_expression;
            }

            const lexer::OperatorType current_operator_type = std::get<lexer::OperatorType>(current_token->data);
            const int current_operator_precedence = get_operator_precedence(current_operator_type);
            if (current_operator_precedence < expression_precedence) {
                return lhs_expression;
            }

            next_token(); // eat binary operator
            auto rhs_expression = parse_primary();
            if (!rhs_expression) {
                return rhs_expression;
                // log(log_type::PARSING_ERROR, "No right hand side expression when parsing binary operator '", current_operator_type, "'");
                // return std::unexpected(ErrorCode::ParserNoRhsValueForBinaryOperator);
            }

            if (current_token->type != lexer::TokenType::Operator) {
                return std::make_shared<ast::BinaryExpression>(current_operator_type, lhs_expression, *rhs_expression);
            }

            const lexer::OperatorType next_operator_type = std::get<lexer::OperatorType>(current_token->data);
            const int next_operator_precedence = get_operator_precedence(next_operator_type);
            if (current_operator_precedence < next_operator_precedence) {
                rhs_expression = parse_binary_expression_rhs(*rhs_expression, current_operator_precedence + 1);
                if (!rhs_expression) {
                    return rhs_expression;
                }
            }

            lhs_expression = std::make_shared<ast::BinaryExpression>(current_operator_type, lhs_expression, *rhs_expression);
        }
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_primary() {
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

        log(log_type::PARSING_ERROR, "Unexpected token '", *current_token, "'");
        return std::unexpected(ErrorCode::ParserUnexpectedToken);
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_identifier() {
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        if (!semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier_id)) {
            log(log_type::PARSING_ERROR, "Undefined symbol '", semantic_analysis::StringTable::get().lookup(identifier_id), "'");
            return std::unexpected(ErrorCode::ParserUndefinedSymbol);
        }

        next_token(); // eat identifier

        // No bracket after identifier -> Variable
        if (current_token->type != lexer::TokenType::BracketOpen) {
            return std::make_shared<ast::VariableExpression>(identifier_id);
        }

        // Bracket after identifier -> Function call
        next_token(); // eat '('
        std::vector<std::shared_ptr<ast::Expression>> args;
        if (current_token->type != lexer::TokenType::BracketClose) {
            while (true) {
                const auto arg = parse_expression();
                if (!arg) {
                    return std::unexpected(arg.error());
                }
                args.push_back(*arg);

                if (current_token->type == lexer::TokenType::BracketClose) {
                    break;
                }

                if (current_token->type != lexer::TokenType::Comma) {
                    log(log_type::PARSING_ERROR, "Expected ')' or ',' in function argument list");
                    return std::unexpected(ErrorCode::ParserUnexpectedToken);
                }

                next_token(); // eat ','
            }
        }

        next_token(); // eat ')'

        return std::make_shared<ast::CallExpression>(identifier_id, std::move(args));
    }

    std::shared_ptr<ast::Expression> Parser::parse_literal() {
        const auto literal_data = current_token->data;
        next_token(); // eat the literal

        if (std::holds_alternative<double>(literal_data)) {
            return std::make_shared<ast::FloatingPointLiteralExpression>(std::get<double>(literal_data));
        } else if (std::holds_alternative<int64_t>(literal_data)) {
            return std::make_shared<ast::IntegerLiteralExpression>(std::get<int64_t>(literal_data));
        } else if (std::holds_alternative<semantic_analysis::StringId>(literal_data)) {
            return std::make_shared<ast::StringLiteralExpression>(std::get<semantic_analysis::StringId>(literal_data));
        } else if (std::holds_alternative<bool>(literal_data)) {
            return std::make_shared<ast::BooleanLiteralExpression>(std::get<bool>(literal_data));
        } else {
            log(log_type::INTERNAL_ERROR, "Literal token doesn't contain the literal data? Tokenization, wtf?");
            emergency_exit();
            std::abort();
        }
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_parenthesis() {
        next_token(); // eat '('

        const auto expression = parse_expression();
        if (!expression) {
            return expression;
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "Expected ')'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ')'
        return expression;
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_negative() {
        next_token(); // eat '-'

        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> expression;
        switch (current_token->type) {
            case lexer::TokenType::Identifier:
                expression = parse_identifier();
                // TODO: Check if the symbol behind the identifier has a type that can be negated
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
                log(log_type::PARSING_ERROR, "Mathematical negation of '", *current_token, "' is not supported");
                return std::unexpected(ErrorCode::ParserUnsupportedMathematicalNegation);
        }

        if (!expression) {
            return expression;
        }
        return std::make_shared<ast::NegationExpression>(*expression);
    }

    std::expected<std::shared_ptr<ast::Expression>, ErrorCode> Parser::parse_cast() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (data_type == type_system::DataTypeKind::Void) {
            log(log_type::PARSING_ERROR, "Cannot cast a value to 'void'");
            return std::unexpected(ErrorCode::ParserInvalidCast);
        }

        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::BracketOpen) {
            log(log_type::PARSING_ERROR, "Expected '(' after data type for cast");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '('
        if (current_token->type == lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "Empty cast");
            return std::unexpected(ErrorCode::ParserInvalidCast);
        }

        const auto expression_to_cast = parse_expression();
        if (!expression_to_cast) {
            return expression_to_cast;
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "Expected ')' after value in cast");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ')'
        return std::make_shared<ast::CastExpression>(*expression_to_cast, data_type);
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::parse_assignment() {
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(); // eat identifier
        if (current_token->type != lexer::TokenType::Assignment) {
            log(log_type::PARSING_ERROR, "Expected '=' after identifier in assignment");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '='
        const auto value_expression = parse_expression();
        if (!value_expression) {
            return std::unexpected(value_expression.error());
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(identifier_id);
        return std::make_shared<ast::AssignmentStatement>(variable, *value_expression);
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::parse_if() {
        const lexer::TokenType if_type = current_token->type;

        next_token(); // eat 'if' or 'elseif'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            log(log_type::PARSING_ERROR, "Expected '(' after '", if_type, "'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '('
        // Parse 'if' condition
        const auto condition = parse_expression();
        if (!condition) {
            return std::unexpected(condition.error());
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            log(log_type::PARSING_ERROR, "Expected ')' after condition in '", if_type, "'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ')'
        // Parse 'if' body
        std::vector<std::shared_ptr<ast::ASTNode>> if_body;
        while (current_token->type != lexer::TokenType::Elseif && current_token->type != lexer::TokenType::Else && current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                log(log_type::PARSING_ERROR, "'if' statement was not closed with an 'end' keyword");
                return std::unexpected(ErrorCode::ParserMissingEndKeyword);
            }

            const auto body_element = parse_statement();
            if (!body_element) {
                return std::unexpected(body_element.error());
            }
            if_body.push_back(*body_element);
        }

        // Setup behaviour for different 'if' ending cases
        if (current_token->type == lexer::TokenType::Elseif) {
            // Don' eat the 'elseif', it will be eaten by in parse_if
            const auto elseif = parse_if();
            if (!elseif) {
                return elseif;
            }
            return std::make_shared<ast::IfStatement>(*condition, std::move(if_body), std::vector<std::shared_ptr<ast::ASTNode>>{*elseif});
        } else if (current_token->type == lexer::TokenType::Else) {
            next_token(); // eat 'else'
            std::vector<std::shared_ptr<ast::ASTNode>> else_body;
            while (current_token->type != lexer::TokenType::End) {
                if (current_token->type == lexer::TokenType::EndOfFile) {
                    log(log_type::PARSING_ERROR, "'else' statement was not closed with an 'end' keyword");
                    return std::unexpected(ErrorCode::ParserMissingEndKeyword);
                }

                const auto body_element = parse_statement();
                if (!body_element) {
                    return std::unexpected(body_element.error());
                }
                else_body.push_back(std::move(*body_element));
            }

            next_token(); // eat 'end'
            return std::make_shared<ast::IfStatement>(*condition, std::move(if_body), std::move(else_body));
        } else if (current_token->type == lexer::TokenType::End) {
            next_token(); // eat 'end'
            return std::make_shared<ast::IfStatement>(*condition, std::move(if_body), std::vector<std::shared_ptr<ast::ASTNode>>{});
        }

        log(log_type::INTERNAL_ERROR, "Don't know how, but I managed to reach the end of the file while parsing an if statement without triggering any of the EOF checks");
        emergency_exit();
        std::abort();
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::parse_for() {
        next_token(); // eat 'for'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            log(log_type::PARSING_ERROR, "Expected '(' after 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '('
        if (current_token->type != lexer::TokenType::DataType) {
            log(log_type::PARSING_ERROR, "Expected data type after '(' in 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (!type_system::is_integer_type(data_type)) {
            log(log_type::PARSING_ERROR, "The loop variable of a 'for' loop has to be an integer, the given type is '", data_type, "'");
            return std::unexpected(ErrorCode::ParserInvalidLoopVariableType);
        }

        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            log(log_type::PARSING_ERROR, "Expected identifier after data type in 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        next_token(); // eat identifier
        if (current_token->type != lexer::TokenType::Colon) {
            log(log_type::PARSING_ERROR, "Expected ':' after identifier in 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ':'
        const auto first_value = parse_expression();
        if (!first_value) {
            return std::unexpected(first_value.error());
        }

        // Only end value is given, start and step are implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, std::make_shared<ast::IntegerLiteralExpression>(0), *first_value, nullptr);
        }

        if (current_token->type != lexer::TokenType::Comma) {
            log(log_type::PARSING_ERROR, "Expected ',' or ')' after first expression in 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ','
        const auto end_value = parse_expression();
        if (!end_value) {
            return std::unexpected(end_value.error());
        }

        // Only start and end value are given, step is implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, *first_value, *end_value, nullptr);
        }

        if (current_token->type != lexer::TokenType::Comma) {
            log(log_type::PARSING_ERROR, "Expected ',' or ')' after second expression in 'for'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat ','
        const auto step_value = parse_expression();
        if (!step_value) {
            return std::unexpected(step_value.error());
        }

        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, *first_value, *end_value, *step_value);
        }

        log(log_type::PARSING_ERROR, "Expected ')' after third expression in 'for'");
        return std::unexpected(ErrorCode::ParserUnexpectedToken);
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::create_for_statement(semantic_analysis::StringId variable_identifier_id, type_system::DataTypeKind variable_data_type, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value) {
        next_token(); // eat ')'
        const auto body = parse_for_body();
        if (!body) {
            return std::unexpected(body.error());
        }

        const auto symbol_id = semantic_analysis::SymbolTable::get().create_variable(variable_identifier_id, variable_data_type);
        if (!symbol_id) {
            return std::unexpected(symbol_id.error());
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(variable_identifier_id);
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = std::make_shared<ast::AssignmentStatement>(variable, start_value);
        const std::shared_ptr<ast::VariableDefinitionStatement> variable_definition_statement = std::make_shared<ast::VariableDefinitionStatement>(variable_data_type, variable_identifier_id, assignment_statement);

        return std::make_shared<ast::ForStatement>(variable_definition_statement, end_value, step_value, std::move(*body));
    }

    std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, ErrorCode> Parser::parse_for_body() {
        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                log(log_type::PARSING_ERROR, "'for' statement was not closed with an 'end' keyword");
                return std::unexpected(ErrorCode::ParserMissingEndKeyword);
            }

            const auto expression = parse_statement();
            if (!expression) {
                return std::unexpected(expression.error());
            }
            body.push_back(*expression);
        }

        next_token(); // eat 'end'
        return body;
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::parse_return() {
        next_token(); // eat 'return' keyword
        if (current_parsing_function_return_type == type_system::DataTypeKind::None) {
            log(log_type::INTERNAL_ERROR, "Someone - and I am not going to say who (maybe because it was myself) - forgot to store the return type of the currently parsed function. And maybe, but just maybe, that's the reason why I'm currently shitting myself trying to parse a return statement without knowing the return type of the function.");
            emergency_exit();
        }

        if (current_parsing_function_return_type == type_system::DataTypeKind::Void) {
            return std::make_shared<ast::ReturnStatement>(nullptr);
        }

        if (current_token->type == lexer::TokenType::End) {
            log(log_type::PARSING_ERROR, "Expected expression after 'return'");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        switch (current_token->type) {
            case lexer::TokenType::If:
            case lexer::TokenType::For:
            case lexer::TokenType::Return:
            case lexer::TokenType::DataType: {
                next_token();
                const lexer::TokenType next_token_type = current_token->type;
                previous_token();
                if (next_token_type != lexer::TokenType::Identifier) {
                    break;
                }

                log(log_type::PARSING_ERROR, "Cannot return '", current_token->type, "' statement");
                return std::unexpected(ErrorCode::ParserInvalidReturnExpression);
            }

            case lexer::TokenType::End:
                log(log_type::PARSING_ERROR, "Expected expression after 'return'");
                return std::unexpected(ErrorCode::ParserUnexpectedToken);

            default: break;
        }

        const auto expression = parse_expression();
        if (!expression) {
            return std::unexpected(expression.error());
        }

        return std::make_shared<ast::ReturnStatement>(*expression);
    }

    std::expected<std::shared_ptr<ast::Statement>, ErrorCode> Parser::parse_variable_declaration() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);

        if (data_type == type_system::DataTypeKind::Void) {
            log(log_type::PARSING_ERROR, "Cannot create a local variable of type 'void'");
            return std::unexpected(ErrorCode::ParserInvalidVariableType);
        }

        next_token(); // eat data type

        if (current_token->type != lexer::TokenType::Identifier) {
            log(log_type::PARSING_ERROR, "Expected identifier after data type for variable declaration");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(); // eat idetifier
        if (current_token->type != lexer::TokenType::Assignment) {
            log(log_type::PARSING_ERROR, "Expected '=' after identifier in variable definition");
            return std::unexpected(ErrorCode::ParserUnexpectedToken);
        }

        next_token(); // eat '='

        const auto expression = parse_expression();
        if (!expression) {
            return std::unexpected(expression.error());
        }

        const auto variable_id = semantic_analysis::SymbolTable::get().create_variable(identifier_id, data_type);
        if (!variable_id) {
            return std::unexpected(variable_id.error());
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(identifier_id);
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = std::make_shared<ast::AssignmentStatement>(variable, *expression);
        return std::make_shared<ast::VariableDefinitionStatement>(data_type, identifier_id, assignment_statement);
    }
}
