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
#include "string_pool.hpp"
#include "type_system/data_type_kind.hpp"
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    std::unique_ptr<Extern> Parser::parse_extern() {
        const SourceLocation& extern_source_location = current_token->source_location;
        next_token(true); // eat 'extern' keyword
        if (current_token->type != TokenType::DataType) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected data type after 'extern'", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        std::unique_ptr<Prototype> prototype = parse_prototype(Prototype::LinkageType::External);
        if (!prototype) {
            return nullptr;
        }
        return std::make_unique<Extern>(std::move(prototype), extern_source_location);
    }

    std::unique_ptr<Prototype> Parser::parse_prototype(Prototype::LinkageType linkage_type) {
        const DataTypeKind return_type = std::get<DataTypeKind>(current_token->data);
        next_token(true); // eat data type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        const SourceLocation& identifier_source_location = current_token->source_location;
        const StringId identifier_id = std::get<StringId>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != TokenType::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type != TokenType::DataType && current_token->type != TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected data type or ')' after '(' in prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        // Parse parameters
        std::vector<ParameterData> parameter_data;
        while (current_token->type == TokenType::DataType) {
            const DataTypeKind parameter_type = std::get<DataTypeKind>(current_token->data);

            next_token(true); // eat data type
            if (current_token->type != TokenType::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after parameter data type", current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            }

            const StringId parameter_identifier_id = std::get<StringId>(current_token->data);
            parameter_data.push_back({.data_type = parameter_type, .identifier_id = identifier_id});

            next_token(true); // eat identifier
            if (current_token->type == TokenType::Comma) {
                next_token(true); // eat ','

                if (current_token->type != TokenType::DataType) {
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        "Expected data type after ',' in prototype parameters",
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                    return nullptr;
                }
            }
        }

        if (current_token->type != TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected ')' after function parameters in prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        next_token(true); // eat ')'

        return std::make_unique<Prototype>(linkage_type, return_type, identifier_id, std::move(parameter_data), identifier_source_location);
    }

    std::unique_ptr<ASTNode> Parser::parse_top_level_data_type() {
        const DataTypeKind data_type = std::get<DataTypeKind>(current_token->data);
        const SourceLocation& data_type_source_location = current_token->source_location;
        next_token(true); // eat data type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after data type on top level", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->type == TokenType::Assignment) {
            diagnostic_sink.report(DiagnosticCode::Unsupported, "Global variables are not supported yet", data_type_source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        // Function definition
        else if (current_token->type == TokenType::BracketOpen) {
            return parse_function(data_type);
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
            "Expected either an assignment operator or a '(' after identifier on top level",
            current_token->source_location);
        recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<Function> Parser::parse_function(DataTypeKind return_type) {
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        previous_token(true);
        const Token* identifier_token = current_token;
        previous_token(true);

        std::unique_ptr<Prototype> prototype = parse_prototype(Prototype::LinkageType::Internal);
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        const std::string_view identifier = StringPool::get().lookup(identifier_id);
        const std::string message = std::format("Function '{}' was not closed with an 'end' keyword", identifier);
        auto body = parse_body<TokenType::End>(message, identifier_token->source_location);
        next_token(true); // eat 'end'

        if (!prototype || !body) {
            return nullptr;
        }

        return std::make_unique<Function>(std::move(prototype), std::move(*body), identifier_token->source_location);
    }

}
