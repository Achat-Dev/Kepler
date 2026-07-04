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
#include "ast/statements/variable_definition_statement.hpp"
#include "diagnostics/diagnostics.hpp"
#include "diagnostics/error_code.hpp"
#include "diagnostics/source_location.hpp"
#include "emergency.hpp"
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

    std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, diagnostics::ErrorCode> Parser::parse() {
        log_verbose("Parsing token stream");

        if (tokens.back().type != lexer::TokenType::EndOfFile) {
            log(log_type::internal_error, "Parser received token stream without EOF token, my tokenizer seems to have fucked up somewhere");
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
                case lexer::TokenType::EndOfFile:
                    log_verbose_no_prefix(log_type::last_indented, "Done");
                    return result;
                case lexer::TokenType::Extern: {
                    const auto ast_node = parse_extern();
                    if (!ast_node) {
                        return std::unexpected(ast_node.error());
                    }
                    result.push_back(*ast_node);
                    break;
                }
                default:
                    return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Unexpected token of type '{}' on top level, expected 'extern' or data type", current_token->type);
            }
        }
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
                log(log_type::internal_error, "Binary operator '", operator_type, "' doesn't have a precedence associated to it\n",
                    log_type::last_indented, "If this was intended, what the fuck where you thinking past me?");
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

    std::expected<std::shared_ptr<ast::ASTNode>, diagnostics::ErrorCode> Parser::parse_extern() {
        next_token(); // eat 'extern' keyword

        if (current_token->type != lexer::TokenType::DataType) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected data type after 'extern'");
        }

        semantic_analysis::SymbolTable::get().open_scope();
        const auto prototype_id = parse_prototype(ast::Prototype::LinkageType::External);
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }
        semantic_analysis::SymbolTable::get().close_scope();

        return std::make_shared<ast::Prototype>(*prototype_id);
    }

    std::expected<semantic_analysis::SymbolId, diagnostics::ErrorCode> Parser::parse_prototype(ast::Prototype::LinkageType linkage_type) {
        const type_system::DataTypeKind return_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected identifier after return type of prototype");
        }
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(); // eat identifier
        if (current_token->type != lexer::TokenType::BracketOpen) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected '(' after prototype name");
        }

        next_token(); // eat '('
        if (current_token->type != lexer::TokenType::DataType && current_token->type != lexer::TokenType::BracketClose) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected data type or ')' after '(' in prototype");
        }

        // Parse parameters
        semantic_analysis::PrototypeSymbolData prototype_symbol_data;
        prototype_symbol_data.linkage_type = linkage_type;
        while (current_token->type == lexer::TokenType::DataType) {
            const type_system::DataTypeKind parameter_type = std::get<type_system::DataTypeKind>(current_token->data);

            next_token(); // eat data type
            if (current_token->type != lexer::TokenType::Identifier) {
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected identifier after parameter data type");
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
                    return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected data type after ',' in prototype parameters");
                }
            }
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')' after function parameters in prototype");
        }
        next_token(); // eat ')'

        const auto prototype_id = semantic_analysis::SymbolTable::get().create_prototype(identifier_id, return_type, std::move(prototype_symbol_data));
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }

        return *prototype_id;
    }

    std::expected<std::shared_ptr<ast::ASTNode>, diagnostics::ErrorCode> Parser::parse_top_level_data_type() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected identifier after data type on top level");
        }

        next_token(); // eat identifier
        // Variable definition
        if (current_token->type == lexer::TokenType::Assignment) {
            if (data_type == type_system::DataTypeKind::Void) {
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserInvalidVariableType, "Cannot create a global variable of type 'void'");
            }
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::Unsupported, "Global variables are not supported yet");
        }
        // Function definition
        else if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_function(data_type);
        }

        return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected either an assignment operator or a '(' after identifier on top level");
    }

    std::expected<std::shared_ptr<ast::Function>, diagnostics::ErrorCode> Parser::parse_function(type_system::DataTypeKind return_type) {
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        previous_token();
        previous_token();

        semantic_analysis::SymbolTable::get().open_scope();

        const auto prototype_id = parse_prototype(ast::Prototype::LinkageType::Internal);
        if (!prototype_id) {
            return std::unexpected(prototype_id.error());
        }

        current_parsing_function_return_type = return_type;

        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserMissingEndKeyword, "Function was not closed with an 'end' keyword");
            }

            const auto statement = parse_statement();
            if (!statement) {
                return std::unexpected(statement.error());
            }
            body.push_back(*statement);
        }

        next_token(); // eat 'end'
        current_parsing_function_return_type = type_system::DataTypeKind::None;
        semantic_analysis::SymbolTable::get().close_scope();

        return std::make_shared<ast::Function>(*prototype_id, std::move(body));
    }

    std::expected<std::shared_ptr<ast::ASTNode>, diagnostics::ErrorCode> Parser::parse_statement() {
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
                    return parse_variable_definition();
                }
                break;
            }
            case lexer::TokenType::Identifier: {
                const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
                if (!semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier_id)) {
                    return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUndefinedSymbol, "Undefined symbol '{}'", semantic_analysis::StringTable::get().lookup(identifier_id));
                }

                next_token(); // eat identifier
                if (current_token->type == lexer::TokenType::Assignment) {
                    return parse_assignment(identifier_id);
                } else if (current_token->type == lexer::TokenType::BracketOpen) {
                    return parse_call(identifier_id);
                }
                previous_token();
                break;
            }

            default:
                break;
        }

        return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Unexpected token '{}'", *current_token);
    }

    std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> Parser::parse_expression() {
        const auto expression = parse_primary();
        if (!expression) {
            return std::unexpected(expression.error());
        }

        if (current_token->type == lexer::TokenType::Operator) {
            return parse_binary_expression_rhs(*expression, 0);
        }
        return expression;
    }

    std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> Parser::parse_binary_expression_rhs(std::shared_ptr<ast::Expression> lhs_expression, int expression_precedence) {
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

    std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> Parser::parse_primary() {
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

        return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Unexpected token '{}'", *current_token);
    }

    std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> Parser::parse_identifier() {
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        if (!semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier_id)) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUndefinedSymbol, "Undefined symbol '{}'", semantic_analysis::StringTable::get().lookup(identifier_id));
        }

        next_token(); // eat identifier
        if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_call(identifier_id);
        }
        return std::make_shared<ast::VariableExpression>(identifier_id);
    }

    std::expected<std::shared_ptr<ast::CallExpression>, diagnostics::ErrorCode> Parser::parse_call(semantic_analysis::StringId identifier_id) {
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
                    return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')' or ',' in function argument list");
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
            log(log_type::internal_error, "Literal token doesn't contain the literal data? Tokenization, wtf?");
            emergency_exit();
            std::abort();
        }
    }

    std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> Parser::parse_parenthesis() {
        next_token(); // eat '('
        const auto expression = parse_expression();
        if (!expression) {
            return expression;
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')'");
        }

        next_token(); // eat ')'
        return expression;
    }

    std::expected<std::shared_ptr<ast::NegationExpression>, diagnostics::ErrorCode> Parser::parse_negative() {
        next_token(); // eat '-'
        std::expected<std::shared_ptr<ast::Expression>, diagnostics::ErrorCode> expression;
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
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnsupportedMathematicalNegation, "Mathematical negation of '{}' is not supported", *current_token);
        }

        if (!expression) {
            return std::unexpected(expression.error());
        }
        return std::make_shared<ast::NegationExpression>(*expression);
    }

    std::expected<std::shared_ptr<ast::CastExpression>, diagnostics::ErrorCode> Parser::parse_cast() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (data_type == type_system::DataTypeKind::Void) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserInvalidCast, "Cannot cast a value to 'void'");
        }

        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::BracketOpen) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected '(' after data type for cast");
        }

        next_token(); // eat '('
        if (current_token->type == lexer::TokenType::BracketClose) {
            diagnostics::SourceLocation source_location{
                current_token->source_location.position - 1,
                2};
            return diagnostics::error(file_path, source_location, diagnostics::ErrorCode::ParserInvalidCast, "Empty cast");
        }

        const auto expression_to_cast = parse_expression();
        if (!expression_to_cast) {
            return std::unexpected(expression_to_cast.error());
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            previous_token();
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')' after value in cast");
        }

        next_token(); // eat ')'
        return std::make_shared<ast::CastExpression>(*expression_to_cast, data_type);
    }

    std::expected<std::shared_ptr<ast::AssignmentStatement>, diagnostics::ErrorCode> Parser::parse_assignment(semantic_analysis::StringId identifier_id) {
        if (current_token->type != lexer::TokenType::Assignment) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected '=' after identifier in assignment");
        }

        next_token(); // eat '='
        const auto value_expression = parse_expression();
        if (!value_expression) {
            return std::unexpected(value_expression.error());
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(identifier_id);
        return std::make_shared<ast::AssignmentStatement>(variable, *value_expression);
    }

    std::expected<std::shared_ptr<ast::IfStatement>, diagnostics::ErrorCode> Parser::parse_if() {
        const lexer::TokenType if_type = current_token->type;

        next_token(); // eat 'if' or 'elseif'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected '(' after '{}'", if_type);
        }

        next_token(); // eat '('
        // Parse 'if' condition
        const auto condition = parse_expression();
        if (!condition) {
            return std::unexpected(condition.error());
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')' after condition in '{}'", if_type);
        }

        next_token(); // eat ')'
        // Parse 'if' body
        std::vector<std::shared_ptr<ast::ASTNode>> if_body;
        while (current_token->type != lexer::TokenType::Elseif && current_token->type != lexer::TokenType::Else && current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserMissingEndKeyword, "'{}' statement was not closed with an 'end' keyword", if_type);
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
                    return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserMissingEndKeyword, "'else' statement was not closed with an 'end' keyword");
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

        log(log_type::internal_error, "Don't know how, but I managed to reach the end of the file while parsing an if statement without triggering any of the EOF checks");
        emergency_exit();
        std::abort();
    }

    std::expected<std::shared_ptr<ast::ForStatement>, diagnostics::ErrorCode> Parser::parse_for() {
        next_token(); // eat 'for'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected '(' after 'for'");
        }

        next_token(); // eat '('
        if (current_token->type != lexer::TokenType::DataType) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected data type after '(' in 'for'");
        }

        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (!type_system::is_integer_type(data_type)) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserInvalidLoopVariableType, "The loop variable of a 'for' loop has to be an integer, the given type is '{}'", data_type);
        }

        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected identifier after data type in 'for'");
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        next_token(); // eat identifier
        if (current_token->type != lexer::TokenType::Colon) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ':' after identifier in 'for'");
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
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ',' or ')' after first expression in 'for'");
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
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ',' or ')' after second expression in 'for'");
        }

        next_token(); // eat ','
        const auto step_value = parse_expression();
        if (!step_value) {
            return std::unexpected(step_value.error());
        }

        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, *first_value, *end_value, *step_value);
        }

        return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected ')' after third expression in 'for'");
    }

    std::expected<std::shared_ptr<ast::ForStatement>, diagnostics::ErrorCode> Parser::create_for_statement(semantic_analysis::StringId variable_identifier_id, type_system::DataTypeKind variable_data_type, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value) {
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

    std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, diagnostics::ErrorCode> Parser::parse_for_body() {
        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserMissingEndKeyword, "'for' statement was not closed with an 'end' keyword");
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

    std::expected<std::shared_ptr<ast::ReturnStatement>, diagnostics::ErrorCode> Parser::parse_return() {
        next_token(); // eat 'return' keyword
        if (current_parsing_function_return_type == type_system::DataTypeKind::None) {
            log(log_type::internal_error, "Someone - and I am not going to say who (maybe because it was myself) - forgot to store the return type of the currently parsed function. And maybe, but just maybe, that's the reason why I'm currently shitting myself trying to parse a return statement without knowing the return type of the function.");
            emergency_exit();
        }

        if (current_parsing_function_return_type == type_system::DataTypeKind::Void) {
            return std::make_shared<ast::ReturnStatement>(nullptr);
        }

        switch (current_token->type) {
            case lexer::TokenType::If:
            case lexer::TokenType::For:
            case lexer::TokenType::Return:
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserInvalidReturnExpression, "Cannot return '{}' statement", current_token->type);
            case lexer::TokenType::DataType: {
                next_token(); // eat data type
                const lexer::TokenType next_token_type = current_token->type;
                previous_token();
                // If this is true it's a cast, which is allowed here
                if (next_token_type == lexer::TokenType::BracketOpen) {
                    break;
                }

                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Unexpected token '{}'", *current_token);
            }

            case lexer::TokenType::End:
                return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected expression after 'return'");

            default: break;
        }

        const auto expression = parse_expression();
        if (!expression) {
            return std::unexpected(expression.error());
        }

        return std::make_shared<ast::ReturnStatement>(*expression);
    }

    std::expected<std::shared_ptr<ast::VariableDefinitionStatement>, diagnostics::ErrorCode> Parser::parse_variable_definition() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);

        if (data_type == type_system::DataTypeKind::Void) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserInvalidVariableType, "Cannot create a local variable of type 'void'");
        }

        next_token(); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            return diagnostics::error(file_path, current_token->source_location, diagnostics::ErrorCode::ParserUnexpectedToken, "Expected identifier after data type for variable declaration");
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        const auto variable_id = semantic_analysis::SymbolTable::get().create_variable(identifier_id, data_type);
        if (!variable_id) {
            return std::unexpected(variable_id.error());
        }

        next_token(); // eat identifier
        const auto assignment_statement = parse_assignment(identifier_id);
        if (!assignment_statement) {
            return std::unexpected(assignment_statement.error());
        }
        return std::make_shared<ast::VariableDefinitionStatement>(data_type, identifier_id, *assignment_statement);
    }
}
