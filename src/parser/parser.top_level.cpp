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
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "semantic_analysis/string_table.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kepler::parser {

    std::shared_ptr<ast::ASTNode> Parser::parse_extern() {
        next_token(true); // eat 'extern' keyword

        if (current_token->type != lexer::TokenType::DataType) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after 'extern'", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        semantic_analysis::SymbolTable::get().open_scope();
        const std::optional<semantic_analysis::SymbolId> prototype_id = parse_prototype(ast::Prototype::LinkageType::External);
        semantic_analysis::SymbolTable::get().close_scope();
        if (!prototype_id) {
            return nullptr; // parse_prototype already recovered, so no need to revocer here
        }

        return std::make_shared<ast::Prototype>(*prototype_id);
    }

    std::optional<semantic_analysis::SymbolId> Parser::parse_prototype(ast::Prototype::LinkageType linkage_type) {
        const type_system::DataTypeKind return_type = std::get<type_system::DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != lexer::TokenType::Identifier) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return std::nullopt;
        }
        const lexer::Token* identifier_token = current_token;
        const semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != lexer::TokenType::BracketOpen) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return std::nullopt;
        }

        next_token(true); // eat '('
        if (current_token->type != lexer::TokenType::DataType && current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type or ')' after '(' in prototype", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return std::nullopt;
        }

        // Parse parameters
        semantic_analysis::PrototypeSymbolData prototype_symbol_data;
        prototype_symbol_data.linkage_type = linkage_type;
        bool did_symbol_creation_fail = false;
        while (current_token->type == lexer::TokenType::DataType) {
            const type_system::DataTypeKind parameter_type = std::get<type_system::DataTypeKind>(current_token->data);

            next_token(true); // eat data type
            if (current_token->type != lexer::TokenType::Identifier) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected identifier after parameter data type", file_path, current_token->source_location);
                recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                return std::nullopt;
            }

            const semantic_analysis::StringId parameter_identifier_id = std::get<semantic_analysis::StringId>(current_token->data);
            const auto parameter_id = semantic_analysis::SymbolTable::get().create_variable(parameter_identifier_id, parameter_type);
            if (!parameter_id) {
                diagnostic_sink.report(diagnostics::DiagnosticCode::SymbolAlreadyExists, parameter_id.error(), file_path, current_token->source_location);
                did_symbol_creation_fail = true;
                // Symbol already exists, but act like it doesn't so the symbols for the following parameters can be created
            } else {
                prototype_symbol_data.parameter_ids.push_back(*parameter_id);
            }

            next_token(true); // eat identifier
            if (current_token->type == lexer::TokenType::Comma) {
                next_token(true); // eat ','

                if (current_token->type != lexer::TokenType::DataType) {
                    diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected data type after ',' in prototype parameters", file_path, current_token->source_location);
                    recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
                    return std::nullopt;
                }
            }
        }

        if (current_token->type != lexer::TokenType::BracketClose) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected ')' after function parameters in prototype", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return std::nullopt;
        }
        next_token(true); // eat ')'

        if (did_symbol_creation_fail) {
            return std::nullopt;
        }

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
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->type == lexer::TokenType::Assignment) {
            diagnostic_sink.report(diagnostics::DiagnosticCode::Unsupported, "Global variables are not supported yet", file_path, current_token->source_location);
            recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
            return nullptr;
        }
        // Function definition
        else if (current_token->type == lexer::TokenType::BracketOpen) {
            return parse_function(data_type);
        }

        diagnostic_sink.report(diagnostics::DiagnosticCode::UnexpectedToken, "Expected either an assignment operator or a '(' after identifier on top level", file_path, current_token->source_location);
        recover(SynchronizationSet<lexer::TokenType::Newline>{}, SynchronizationSet<lexer::TokenType::Newline>{});
        return nullptr;
    }

    std::shared_ptr<ast::Function> Parser::parse_function(type_system::DataTypeKind return_type) {
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        const size_t function_definition_end_position = current_token->source_location.position + current_token->source_location.size;
        previous_token(true);
        previous_token(true);
        const size_t current_position = current_token->source_location.position;
        diagnostics::SourceLocation function_definition_source_location(current_position, function_definition_end_position - current_position - 1);

        semantic_analysis::SymbolTable::get().open_scope();
        current_parsing_function_return_type = return_type;

        const auto prototype_id = parse_prototype(ast::Prototype::LinkageType::Internal);
        const auto body = parse_body<lexer::TokenType::End>("Function was not closed with an 'end' keyword", function_definition_source_location);
        next_token(true); // eat 'end'
        current_parsing_function_return_type = type_system::DataTypeKind::None;
        semantic_analysis::SymbolTable::get().close_scope();

        if (!prototype_id || !body) {
            return nullptr;
        }

        return std::make_shared<ast::Function>(*prototype_id, std::move(*body));
    }

}
