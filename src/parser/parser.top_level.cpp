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
#include "ast/extern.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kepler {

    std::unique_ptr<Extern> Parser::parse_extern() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Extern,
            "Parsing extern requires current token to be of type '{}', received '{}'",
            TokenType::Extern,
            current_token->type);

        const SourceLocation& extern_source_location = current_token->source_location;
        next_token(true); // eat 'extern' keyword
        if (current_token->type != TokenType::Type) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type after 'extern'", current_token->source_location);
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
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Type,
            "Parsing prototype requires current token to be of type '{}', received '{}'",
            TokenType::Type,
            current_token->type);
        KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Prototype type token");
        const StringId return_type_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        const SourceLocation& identifier_source_location = current_token->source_location;
        KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Prototype identifier token");
        const StringId identifier_id = std::get<StringId>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != TokenType::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type != TokenType::Type && current_token->type != TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type or ')' after '(' in prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        // Parse parameters
        std::vector<ParameterData> parameter_data;
        while (current_token->type == TokenType::Type) {
            KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Prototype parameter type token");
            const StringId parameter_type_id = std::get<StringId>(current_token->data);

            next_token(true); // eat type
            if (current_token->type != TokenType::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after parameter type", current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            }

            KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Prototype parameter identifier token");
            const StringId parameter_identifier_id = std::get<StringId>(current_token->data);
            parameter_data.push_back({.type_id = parameter_type_id,
                .identifier_id = parameter_identifier_id,
                .identifier_source_location = current_token->source_location});

            next_token(true); // eat identifier
            if (current_token->type == TokenType::Comma) {
                next_token(true); // eat ','

                if (current_token->type != TokenType::Type) {
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        "Expected type after ',' in prototype parameters",
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

        return std::make_unique<Prototype>(linkage_type,
            return_type_id,
            identifier_id,
            std::move(parameter_data),
            type_source_location,
            identifier_source_location);
    }

    std::unique_ptr<ASTNode> Parser::parse_top_level_type() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Type,
            "Parsing top level type requires current token to be of type '{}', received '{}'",
            TokenType::Type,
            current_token->type);
        KPL_ASSERT_HOLDS_ALTERNATIVE(current_token->data, StringId, "Top level type token");
        const StringId type_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after type on top level", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->type == TokenType::Assignment) {
            diagnostic_sink.report(DiagnosticCode::Unsupported, "Global variables are not supported yet", type_source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        // Function definition
        else if (current_token->type == TokenType::BracketOpen) {
            return parse_function();
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
            "Expected either an assignment operator or a '(' after identifier on top level",
            current_token->source_location);
        recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<Function> Parser::parse_function() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::BracketOpen,
            "Parsing a function requires token of type '{}', received '{}'",
            TokenType::BracketOpen,
            current_token->type);
        KPL_ASSERT_THAT(!current_function_return_type_id.has_value(), "Function return type id must be nullopt for parsing a function");
        // Current token is '(', so go back by two (identifier and return type) so the prototype of the function can be parsed
        previous_token(true);
        const Token* identifier_token = current_token;
        previous_token(true);
        std::unique_ptr<Prototype> prototype = parse_prototype(Prototype::LinkageType::Internal);
        current_function_return_type_id = prototype->return_type_id;

        KPL_ASSERT_HOLDS_ALTERNATIVE(identifier_token->data, StringId, "Function identifier token");
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        const std::string_view identifier = StringPool::get().lookup(identifier_id);
        const std::string message = std::format("Function '{}' was not closed with an 'end' keyword", identifier);
        auto body = parse_body<TokenType::End>(message, identifier_token->source_location);
        next_token(true); // eat 'end' or 'EOF'

        current_function_return_type_id = std::nullopt;
        if (!prototype || !body) {
            return nullptr;
        }

        return std::make_unique<Function>(std::move(prototype), std::move(*body), identifier_token->source_location);
    }

}
