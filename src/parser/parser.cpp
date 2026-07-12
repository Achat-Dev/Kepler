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
#include "diagnostics/diagnostic_code.hpp"
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
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler::parser {

    std::vector<std::shared_ptr<ast::ASTNode>> Parser::parse() {
        log::verbose("Parsing token stream");

        if (tokens.back().type != lexer::TokenType::EndOfFile) {
            emergency_exit("Parser received token stream without EOF token, my tokenizer seems to have fucked up somewhere");
        }

        std::vector<std::shared_ptr<ast::ASTNode>> result;
        while (true) {
            switch (current_token->type) {
                case lexer::TokenType::DataType: {
                    const std::shared_ptr<ast::ASTNode> ast_node = parse_top_level_data_type();
                    if (!ast_node) {
                        break;
                    }
                    result.push_back(ast_node);
                    break;
                }
                case lexer::TokenType::EndOfFile:
                    log::verbose_no_prefix("{} Parsing done", log::styling::last_indented);
                    return result;
                case lexer::TokenType::Extern: {
                    const std::shared_ptr<ast::ASTNode> ast_node = parse_extern();
                    if (!ast_node) {
                        break;
                    }
                    result.push_back(ast_node);
                    break;
                }
                case lexer::TokenType::Newline:
                    next_token(true);
                    break;
                default:
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}' on top level, expected 'extern' or function definition", current_token->type), file_path, current_token->source_location);
                    recover(true, lexer::TokenType::Newline);
                    break;
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

    void Parser::next_token(bool skip_newline) {
        if (current_token_index < tokens.size() - 1) {
            current_token_index++;
            current_token = &tokens[current_token_index];

            if (current_token->type == lexer::TokenType::Newline && skip_newline) {
                next_token(skip_newline);
            }
        }
    }

    void Parser::previous_token(bool skip_newline) {
        if (current_token_index > 0) {
            current_token_index--;
            current_token = &tokens[current_token_index];

            if (current_token->type == lexer::TokenType::Newline && skip_newline) {
                previous_token(skip_newline);
            }
        }
    }

    std::shared_ptr<ast::ASTNode> Parser::parse_extern() {
        next_token(true); // eat 'extern' keyword

        if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after 'extern'", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        semantic_analysis::SymbolTable::get().open_scope();
        const std::optional<semantic_analysis::SymbolId> prototype_id = parse_prototype(ast::Prototype::LinkageType::External);
        semantic_analysis::SymbolTable::get().close_scope();
        if (!prototype_id) {
            return nullptr;
        }

        return std::make_shared<ast::Prototype>(*prototype_id);
    }

    std::optional<semantic_analysis::SymbolId> Parser::parse_prototype(ast::Prototype::LinkageType linkage_type) {
        const type_system::DataTypeKind return_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return std::nullopt;
        }
        const lexer::Token* identifier_token = current_token;
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return std::nullopt;
        }

        next_token(true); // eat '('
        if (current_token->type != lexer::TokenType::DataType && current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type or ')' after '(' in prototype", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline, lexer::TokenType::Comma, lexer::TokenType::BracketClose);
        }

        // Parse parameters
        semantic_analysis::PrototypeSymbolData prototype_symbol_data;
        prototype_symbol_data.linkage_type = linkage_type;
        while (current_token->type == lexer::TokenType::DataType) {
            const type_system::DataTypeKind parameter_type = std::get<type_system::DataTypeKind>(current_token->data);

            next_token(true); // eat data type
            if (current_token->type != lexer::TokenType::Identifier) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after parameter data type", file_path, current_token->source_location);
                recover(false, lexer::TokenType::Newline, lexer::TokenType::Comma, lexer::TokenType::BracketClose);
                if (current_token->type == lexer::TokenType::Comma) {
                    next_token(true);
                }
                continue;
            }

            const semantic_analysis::StringId parameter_identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
            const auto parameter_id = semantic_analysis::SymbolTable::get().create_variable(parameter_identifier_id, parameter_type);
            if (!parameter_id) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, parameter_id.error(), file_path, current_token->source_location);
                recover(false, lexer::TokenType::Newline, lexer::TokenType::Comma, lexer::TokenType::BracketClose);
                if (current_token->type == lexer::TokenType::Comma) {
                    next_token(true);
                }
                continue;
            }
            prototype_symbol_data.parameter_ids.push_back(*parameter_id);

            next_token(true); // eat identifier
            if (current_token->type == lexer::TokenType::Comma) {
                next_token(true); // eat ','

                if (current_token->type != lexer::TokenType::DataType) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after ',' in prototype parameters", file_path, current_token->source_location);
                    recover(false, lexer::TokenType::Newline, lexer::TokenType::Comma, lexer::TokenType::BracketClose);
                    if (current_token->type == lexer::TokenType::Comma) {
                        next_token(true);
                    }
                    continue;
                }
            }
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after function parameters in prototype", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline, lexer::TokenType::BracketClose);
            return std::nullopt;
        }
        next_token(true); // eat ')'

        const auto prototype_id = semantic_analysis::SymbolTable::get().create_prototype(identifier_id, return_type, std::move(prototype_symbol_data));
        if (!prototype_id) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, prototype_id.error(), file_path, identifier_token->source_location);
            return std::nullopt;
        }

        return *prototype_id;
    }

    std::shared_ptr<ast::ASTNode> Parser::parse_top_level_data_type() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type on top level", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->type == lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::Unsupported, "Global variables are not supported yet", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }
        // Function definition
        else if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_function(data_type);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected either an assignment operator or a '(' after identifier on top level", file_path, current_token->source_location);
        recover(true, lexer::TokenType::Newline);
        return nullptr;
    }

    std::shared_ptr<ast::Function> Parser::parse_function(type_system::DataTypeKind return_type) {
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        const size_t function_definition_size = current_token->source_location.position + current_token->source_location.size;
        previous_token(true);
        previous_token(true);
        diagnostics::SourceLocation function_definition_location(current_token->source_location.position, function_definition_size);

        semantic_analysis::SymbolTable::get().open_scope();

        current_parsing_function_return_type = return_type;
        const std::optional<semantic_analysis::SymbolId> prototype_id = parse_prototype(ast::Prototype::LinkageType::Internal);

        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, "Function was not closed with an 'end' keyword", file_path, function_definition_location);
                semantic_analysis::SymbolTable::get().close_scope();
                return nullptr; // EOF, so there is no need to recover
            }

            const std::shared_ptr<ast::ASTNode> statement = parse_statement();
            if (statement) {
                body.push_back(statement);
            }
        }

        next_token(true); // eat 'end'
        current_parsing_function_return_type = type_system::DataTypeKind::None;
        semantic_analysis::SymbolTable::get().close_scope();

        if (!prototype_id) {
            return nullptr;
        }

        return std::make_shared<ast::Function>(*prototype_id, std::move(body));
    }

    std::shared_ptr<ast::ASTNode> Parser::parse_statement() {
        switch (current_token->type) {
            case lexer::TokenType::If:
                return parse_if();
            case lexer::TokenType::For:
                return parse_for();
            case lexer::TokenType::Return:
                return parse_return();
            case lexer::TokenType::DataType: {
                next_token(true); // eat data type
                if (current_token->type == lexer::TokenType::Identifier) {
                    previous_token(true);
                    return parse_variable_definition();
                }
                break;
            }
            case lexer::TokenType::Identifier: {
                const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
                const bool is_undefined_symbol = !semantic_analysis::SymbolTable::get().does_name_exist_in_scope_stack(identifier_id);
                const diagnostics::SourceLocation identifier_source_location = current_token->source_location;

                next_token(true); // eat identifier
                if (current_token->type == lexer::TokenType::Assignment) {
                    if (is_undefined_symbol) {
                        const std::string message = std::format("Undefined symbol '{}'", semantic_analysis::StringTable::get().lookup(identifier_id));
                        diagnostic_sink.report(diagnostics::DiagnosticCode::UndefinedSymbol, message, file_path, identifier_source_location);
                        // Symbol doesn't exist, but act like it exists for further diagnostics, so no need to recover and/or return
                    }
                    return parse_assignment(identifier_id);
                } else if (current_token->type == lexer::TokenType::BracketOpen) {
                    if (is_undefined_symbol) {
                        const std::string message = std::format("Undefined symbol '{}'", semantic_analysis::StringTable::get().lookup(identifier_id));
                        diagnostic_sink.report(diagnostics::DiagnosticCode::UndefinedSymbol, message, file_path, identifier_source_location);
                        // Symbol doesn't exist, but act like it exists for further diagnostics, so no need to recover and/or return
                    }
                    return parse_call(identifier_id);
                }

                previous_token(true); // Previous token for the diagnostic
                break;
            }

            default:
                break;
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), file_path, current_token->source_location);
        recover(true, lexer::TokenType::Newline);
        return nullptr;
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
        recover(true, lexer::TokenType::Newline);
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
        std::vector<std::shared_ptr<ast::Expression>> args;
        if (current_token->type != lexer::TokenType::BracketClose) {
            while (true) {
                const std::shared_ptr<ast::Expression> arg = parse_expression();
                if (arg) {
                    args.push_back(arg);
                }

                if (current_token->type == lexer::TokenType::BracketClose) {
                    break;
                }
                if (current_token->type != lexer::TokenType::Comma) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' or ',' in function argument list", file_path, current_token->source_location);
                    recover(false, lexer::TokenType::Newline, lexer::TokenType::Comma, lexer::TokenType::BracketClose);
                }

                next_token(true); // eat ','
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
            return expression;
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::BracketClose);
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
                previous_token(true);
                const std::string message = std::format("Mathematical negation of '{}' is not supported", *current_token);
                diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidMathematicalNegation, message, file_path, current_token->source_location);
                recover(true, lexer::TokenType::Newline);
                return nullptr;
        }

        if (!expression) {
            return nullptr;
        }
        return std::make_shared<ast::NegationExpression>(expression);
    }

    std::shared_ptr<ast::CastExpression> Parser::parse_cast() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (data_type == type_system::DataTypeKind::Void) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidCast, "Cannot cast a value to 'void'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::BracketOpen);
            if (current_token->type == lexer::TokenType::Newline) {
                next_token(true);
                return nullptr;
            }
        }

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after data type for cast", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::BracketOpen);
            if (current_token->type == lexer::TokenType::Newline) {
                next_token(true);
                return nullptr;
            }
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
            previous_token(true);
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after value in cast", file_path, current_token->source_location);
            next_token(false);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::BracketClose);
        }

        next_token(true); // eat ')'
        return std::make_shared<ast::CastExpression>(expression_to_cast, data_type);
    }

    std::shared_ptr<ast::AssignmentStatement> Parser::parse_assignment(semantic_analysis::StringId identifier_id) {
        if (current_token->type != lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '=' after identifier in assignment", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        next_token(true); // eat '='
        const std::shared_ptr<ast::Expression> value_expression = parse_expression();
        if (!value_expression) {
            return nullptr;
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(identifier_id);
        return std::make_shared<ast::AssignmentStatement>(variable, value_expression);
    }

    std::shared_ptr<ast::IfStatement> Parser::parse_if() {
        const lexer::TokenType if_type = current_token->type;

        std::shared_ptr<ast::Expression> condition = nullptr;
        next_token(true); // eat 'if' or 'elseif'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            const std::string message = std::format("Expected '(' after '{}'", if_type);
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
        } else {
            next_token(true); // eat '('
            // Parse 'if' condition
            condition = parse_expression();

            if (current_token->type != lexer::TokenType::BracketClose) {
                const std::string message = std::format("Expected ')' after condition in '{}'", if_type);
                previous_token(true);
                diagnostics::SourceLocation source_location(current_token->source_location.position + 1, 1);
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, message, file_path, source_location);
                next_token(false);
                recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
                if (current_token->type == lexer::TokenType::End) {
                    next_token(true);
                    return nullptr;
                }
            }
        }
        next_token(true); // eat ')'

        // Parse 'if' body
        std::vector<std::shared_ptr<ast::ASTNode>> if_body;
        while (current_token->type != lexer::TokenType::Elseif && current_token->type != lexer::TokenType::Else && current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                const std::string message = std::format("'{}' statement was not closed with an 'end' keyword", if_type);
                diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, message, file_path, current_token->source_location);
                return nullptr; // EOF, so there is no need to recover
            }

            const std::shared_ptr<ast::ASTNode> body_element = parse_statement();
            if (body_element) {
                if_body.push_back(body_element);
            }
        }

        // Setup behaviour for different 'if' ending cases
        if (current_token->type == lexer::TokenType::Elseif) {
            // Don' eat the 'elseif', it will be eaten by in parse_if
            const std::shared_ptr<ast::IfStatement> elseif = parse_if();
            if (condition && elseif) {
                return std::make_shared<ast::IfStatement>(condition, std::move(if_body), std::vector<std::shared_ptr<ast::ASTNode>>{elseif});
            }
        } else if (current_token->type == lexer::TokenType::Else) {
            next_token(true); // eat 'else'
            std::vector<std::shared_ptr<ast::ASTNode>> else_body;
            while (current_token->type != lexer::TokenType::End) {
                if (current_token->type == lexer::TokenType::EndOfFile) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, "'else' statement was not closed with an 'end' keyword", file_path, current_token->source_location);
                    return nullptr; // EOF, so there is no need to recover
                }

                const std::shared_ptr<ast::ASTNode> body_element = parse_statement();
                if (body_element) {
                    else_body.push_back(std::move(body_element));
                }
            }

            next_token(true); // eat 'end'

            if (condition) {
                return std::make_shared<ast::IfStatement>(condition, std::move(if_body), std::move(else_body));
            }
        } else if (current_token->type == lexer::TokenType::End) {
            next_token(true); // eat 'end'
            if (condition) {
                return std::make_shared<ast::IfStatement>(condition, std::move(if_body), std::vector<std::shared_ptr<ast::ASTNode>>{});
            }
        }

        return nullptr;
    }

    std::shared_ptr<ast::ForStatement> Parser::parse_for() {
        next_token(true); // eat 'for'
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after '(' in 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        if (!type_system::is_integer_type(data_type)) {
            const std::string message = std::format("The loop variable of a 'for' loop has to be an integer, the given type is '{}'", data_type);
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidLoopVariableType, message, file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type in 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::Colon) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ':' after identifier in 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        next_token(true); // eat ':'
        const std::shared_ptr<ast::Expression> first_value = parse_expression();
        if (!first_value) {
            previous_token(true);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        // Only end value is given, start and step are implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, std::make_shared<ast::IntegerLiteralExpression>(0), first_value, nullptr);
        }

        if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after first expression in 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        next_token(true); // eat ','
        const std::shared_ptr<ast::Expression> end_value = parse_expression();
        if (!end_value) {
            previous_token(true);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        // Only start and end value are given, step is implicit
        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, first_value, end_value, nullptr);
        }

        if (current_token->type != lexer::TokenType::Comma) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ',' or ')' after second expression in 'for'", file_path, current_token->source_location);
            recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        next_token(true); // eat ','
        const std::shared_ptr<ast::Expression> step_value = parse_expression();
        if (!step_value) {
            previous_token(true);
            if (current_token->type == lexer::TokenType::End) {
                next_token(true);
                return nullptr;
            }
            next_token(true);
            parse_for_body();
            return nullptr;
        }

        if (current_token->type == lexer::TokenType::BracketClose) {
            return create_for_statement(identifier_id, data_type, first_value, end_value, step_value);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after third expression in 'for'", file_path, current_token->source_location);
        recover(false, lexer::TokenType::Newline, lexer::TokenType::End);
        if (current_token->type == lexer::TokenType::End) {
            next_token(true);
            return nullptr;
        }
        next_token(true);
        parse_for_body();
        return nullptr;
    }

    std::shared_ptr<ast::ForStatement> Parser::create_for_statement(semantic_analysis::StringId variable_identifier_id, type_system::DataTypeKind variable_data_type, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value) {
        next_token(true); // eat ')'
        const std::vector<std::shared_ptr<ast::ASTNode>> body = parse_for_body();
        const auto symbol_id = semantic_analysis::SymbolTable::get().create_variable(variable_identifier_id, variable_data_type);
        if (!symbol_id) {
            // Backtrack until the for keyword and then go to the variable definition for the diagnostics
            while (current_token->type != lexer::TokenType::For) {
                previous_token(true);
            }
            next_token(true); // eat 'for'
            next_token(true); // eat '('
            next_token(true); // eat data type

            diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, symbol_id.error(), file_path, current_token->source_location);
            recover(true, lexer::TokenType::End);
            return nullptr;
        }

        const std::shared_ptr<ast::VariableExpression> variable = std::make_shared<ast::VariableExpression>(variable_identifier_id);
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = std::make_shared<ast::AssignmentStatement>(variable, start_value);
        const std::shared_ptr<ast::VariableDefinitionStatement> variable_definition_statement = std::make_shared<ast::VariableDefinitionStatement>(variable_data_type, variable_identifier_id, assignment_statement);

        return std::make_shared<ast::ForStatement>(variable_definition_statement, end_value, step_value, std::move(body));
    }

    std::vector<std::shared_ptr<ast::ASTNode>> Parser::parse_for_body() {
        std::vector<std::shared_ptr<ast::ASTNode>> body;
        while (current_token->type != lexer::TokenType::End) {
            if (current_token->type == lexer::TokenType::EndOfFile) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::MissingEndKeyword, "'for' statement was not closed with an 'end' keyword", file_path, current_token->source_location);
                return body;
            }

            const std::shared_ptr<ast::ASTNode> statement = parse_statement();
            if (statement) {
                body.push_back(statement);
            }
        }

        next_token(true); // eat 'end'
        return body;
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
                recover(true, lexer::TokenType::Newline);
                return nullptr;
            }
            case lexer::TokenType::DataType: {
                next_token(true); // eat data type
                // If this is true it's a cast, which is allowed here
                if (current_token->type == lexer::TokenType::BracketOpen) {
                    previous_token(true);
                    break;
                }

                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, std::format("Unexpected token '{}'", *current_token), file_path, current_token->source_location);
                recover(true, lexer::TokenType::Newline);
                return nullptr;
            }

            case lexer::TokenType::End:
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected expression after 'return'", file_path, current_token->source_location);
                next_token(true); // eat 'end'
                return nullptr;

            default: break;
        }

        const std::shared_ptr<ast::Expression> expression = parse_expression();
        if (!expression) {
            return nullptr;
        }

        return std::make_shared<ast::ReturnStatement>(expression);
    }

    std::shared_ptr<ast::VariableDefinitionStatement> Parser::parse_variable_definition() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);

        if (data_type == type_system::DataTypeKind::Void) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::InvalidVariableType, "Cannot create a local variable of type 'void'", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type for variable declaration", file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
        const auto variable_id = semantic_analysis::SymbolTable::get().create_variable(identifier_id, data_type);
        if (!variable_id) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, variable_id.error(), file_path, current_token->source_location);
            recover(true, lexer::TokenType::Newline);
            return nullptr;
        }

        next_token(true); // eat identifier
        const std::shared_ptr<ast::AssignmentStatement> assignment_statement = parse_assignment(identifier_id);
        if (!assignment_statement) {
            return nullptr;
        }
        return std::make_shared<ast::VariableDefinitionStatement>(data_type, identifier_id, assignment_statement);
    }
}
