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
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kepler::parser {

    std::unique_ptr<ast::Extern> Parser::parse_extern() {
        const diagnostics::SourceLocation& extern_source_location = current_token->source_location;
        next_token(true); // eat 'extern' keyword
        if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after 'extern'", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        std::unique_ptr<ast::Prototype> prototype = parse_prototype(ast::Prototype::LinkageType::External);
        if (!prototype) {
            return nullptr;
        }
        return std::make_unique<ast::Extern>(std::move(prototype), extern_source_location);
    }

    std::unique_ptr<ast::Prototype> Parser::parse_prototype(ast::Prototype::LinkageType linkage_type) {
        const type_system::DataTypeKind return_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }
        const diagnostics::SourceLocation& identifier_source_location = current_token->source_location;
        const std::string& identifier = std::get<std::string>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type != lexer::TokenType::DataType && current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type or ')' after '(' in prototype", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        // Parse parameters
        std::vector<ast::ParameterData> parameter_data;
        while (current_token->type == lexer::TokenType::DataType) {
            const type_system::DataTypeKind parameter_type = std::get<type_system::DataTypeKind>(current_token->data);

            next_token(true); // eat data type
            if (current_token->type != lexer::TokenType::Identifier) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after parameter data type", current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return nullptr;
            }

            const std::string& parameter_identifier = std::get<std::string>(current_token->data);
            parameter_data.push_back({.data_type = parameter_type, .identifier = identifier});

            next_token(true); // eat identifier
            if (current_token->type == lexer::TokenType::Comma) {
                next_token(true); // eat ','

                if (current_token->type != lexer::TokenType::DataType) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after ',' in prototype parameters", current_token->source_location);
                    recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                    return nullptr;
                }
            }
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after function parameters in prototype", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }
        next_token(true); // eat ')'

        return std::make_unique<ast::Prototype>(linkage_type, return_type, identifier, std::move(parameter_data), identifier_source_location);
    }

    std::unique_ptr<ast::ASTNode> Parser::parse_top_level_data_type() {
        const type_system::DataTypeKind data_type = std::get<type_system::DataTypeKind>(current_token->data);
        const diagnostics::SourceLocation& data_type_source_location = current_token->source_location;
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after data type on top level", current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->type == lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::Unsupported, "Global variables are not supported yet", data_type_source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }
        // Function definition
        else if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_function(data_type);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected either an assignment operator or a '(' after identifier on top level", current_token->source_location);
        recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<ast::Function> Parser::parse_function(type_system::DataTypeKind return_type) {
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        previous_token(true);
        const lexer::Token* identifier_token = current_token;
        previous_token(true);

        std::unique_ptr<ast::Prototype> prototype = parse_prototype(ast::Prototype::LinkageType::Internal);
        const std::string message = std::format("Function '{}' was not closed with an 'end' keyword", std::get<std::string>(identifier_token->data));
        auto body = parse_body<lexer::TokenType::End>(message, identifier_token->source_location);
        next_token(true); // eat 'end'

        if (!prototype || !body) {
            return nullptr;
        }

        return std::make_unique<ast::Function>(std::move(prototype), std::move(*body), identifier_token->source_location);
    }

}
