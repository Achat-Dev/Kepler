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
#include "ast/statements/import_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "ast/struct.hpp"
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
#include <variant>
#include <vector>

namespace kepler {

    std::unique_ptr<ModuleStatement> Parser::parse_module() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Module, "Required token: '{}', received: '{}'", TokenType::Module, current_token->type);
        const SourceLocation& module_source_location = current_token->source_location;
        next_token(true); // eat 'module' keyword
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'module'", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        const auto module_path_parse_result = parse_identifier_path(TokenType::DoubleColon,
            IdentifierPathParseKind::IncludeLastIdentifier,
            "in module path");
        if (!module_path_parse_result.has_value()) {
            return nullptr;
        }
        return std::make_unique<ModuleStatement>(std::move(module_path_parse_result.value().identifier_path), module_source_location);
    }

    std::unique_ptr<ImportStatement> Parser::parse_import() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Import, "Required token: '{}', received: '{}'", TokenType::Import, current_token->type);
        const SourceLocation& import_source_location = current_token->source_location;
        next_token(true); // eat 'import' keyword
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'import'", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        const auto module_path_parse_result = parse_identifier_path(TokenType::DoubleColon,
            IdentifierPathParseKind::IncludeLastIdentifier,
            "in imported module path");
        if (!module_path_parse_result.has_value()) {
            return nullptr;
        }
        return std::make_unique<ImportStatement>(std::move(module_path_parse_result.value().identifier_path), import_source_location);
    }

    std::unique_ptr<ExportableNode> Parser::parse_export() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Export, "Required token: '{}', received: '{}'", TokenType::Export, current_token->type);
        next_token(true); // eat 'export'
        switch (current_token->type) {
            case TokenType::Extern:
                return parse_extern(LinkageType::Export);
                break;
            case TokenType::Struct:
                return parse_struct(LinkageType::Export);
                break;
            case TokenType::Identifier:
                return parse_top_level_identifier(LinkageType::Export);
                break;
            default:
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected 'extern' or type after 'export'", current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
        }
    }

    std::unique_ptr<Extern> Parser::parse_extern(LinkageType linkage_type) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Extern, "Required token: '{}', received: '{}'", TokenType::Extern, current_token->type);

        const SourceLocation& extern_source_location = current_token->source_location;
        next_token(true); // eat 'extern' keyword
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type after 'extern'", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        std::unique_ptr<Prototype> prototype = parse_prototype();
        if (!prototype) {
            return nullptr;
        }
        return std::make_unique<Extern>(std::move(prototype), linkage_type, extern_source_location);
    }

    std::unique_ptr<Prototype> Parser::parse_prototype() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Identifier, "Required token: '{}', received: '{}'", TokenType::Identifier, current_token->type);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId return_type_identifier_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type identifier
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        const SourceLocation& identifier_source_location = current_token->source_location;
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId identifier_id = std::get<StringId>(current_token->data);

        next_token(true); // eat identifier
        if (current_token->type != TokenType::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after prototype name", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
        if (current_token->type != TokenType::Identifier && current_token->type != TokenType::Variadic && current_token->type != TokenType::BracketClose) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type or ')' after '(' in prototype", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        // Parse parameters
        std::vector<ParameterData> parameter_data;
        bool is_variadic = false;
        while (current_token->type == TokenType::Identifier) {
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            const StringId parameter_type_id = std::get<StringId>(current_token->data);

            next_token(true); // eat type identifier
            if (current_token->type != TokenType::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after parameter type", current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return nullptr;
            }

            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            const StringId parameter_identifier_id = std::get<StringId>(current_token->data);
            parameter_data.push_back({.type_identifier_id = parameter_type_id,
                .identifier_id = parameter_identifier_id,
                .identifier_source_location = current_token->source_location});

            next_token(true); // eat identifier
            if (current_token->type == TokenType::Comma) {
                next_token(true); // eat ','
                // Lookahead for correct diagnostic
                if (current_token->type != TokenType::Identifier && current_token->type != TokenType::Variadic) {
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        "Expected type after ',' in prototype parameters",
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                    return nullptr;
                }
            }
        }

        // Check for variadic function
        if (current_token->type == TokenType::Variadic) {
            next_token(true); // eat '...'
            is_variadic = true;
        }

        if (current_token->type != TokenType::BracketClose) {
            std::string message;
            if (is_variadic) {
                message = "Expected ')' after variadic parameter in prototype (variadic must be the last parameter)";
            } else {
                message = "Expected ')' after function parameters in prototype";
            }
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::move(message), current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }
        next_token(true); // eat ')'

        return std::make_unique<Prototype>(return_type_identifier_id,
            identifier_id,
            std::move(parameter_data),
            is_variadic,
            type_source_location,
            identifier_source_location);
    }

    std::unique_ptr<Struct> Parser::parse_struct(LinkageType linkage_type) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Struct);
        const SourceLocation& struct_source_location = current_token->source_location;
        next_token(true); // eat 'struct' keyword
        if (current_token->type != TokenType::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'struct'", current_token->source_location);
            recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
            return nullptr;
        }

        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const Token* identifier_token = current_token;
        next_token(true); // eat identifier

        std::vector<StructNodeMember> members;
        while (current_token->type != TokenType::End) {
            if (current_token->type == TokenType::EndOfFile) {
                const StringId identifier_id = std::get<StringId>(identifier_token->data);
                const std::string message = std::format("struct '{}' was not closed with an 'end' keyword", StringPool::get().lookup(identifier_id));
                diagnostic_sink.report(DiagnosticCode::MissingEndKeyword, std::move(message), struct_source_location);
                return nullptr;
            }

            if (current_token->type != TokenType::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type for struct member", current_token->source_location);
                recover(SynchronizationSet<TokenType::End>{}, SynchronizationSet<>{});
                continue;
            }
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            const Token* member_type_token = current_token;

            next_token(true); // eat type identifier
            if (current_token->type != TokenType::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after type for struct member", current_token->source_location);
                recover(SynchronizationSet<TokenType::End>{}, SynchronizationSet<>{});
                continue;
            }
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            members.push_back({
                .type_identifier_id = std::get<StringId>(member_type_token->data),
                .identifier_id = std::get<StringId>(current_token->data),
                .type_source_location = member_type_token->source_location,
                .identifier_source_location = current_token->source_location,
            });
            next_token(true); // eat identifier
        }

        next_token(true); // eat 'end'
        return std::make_unique<Struct>(std::get<StringId>(identifier_token->data),
            std::move(members),
            linkage_type,
            identifier_token->source_location);
    }

    std::unique_ptr<ExportableNode> Parser::parse_top_level_identifier(LinkageType linkage_type) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Identifier, "Required token: '{}', received: '{}'", TokenType::Identifier, current_token->type);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId type_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type identifier
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
            previous_token(true); // Go back to identifier
            previous_token(true); // Go back to type
            return parse_function(linkage_type);
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
            "Expected either an assignment operator or a '(' after identifier on top level",
            current_token->source_location);
        recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
        return nullptr;
    }

    std::unique_ptr<Function> Parser::parse_function(LinkageType linkage_type) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->type == TokenType::Identifier, "Required token: '{}', received: '{}'", TokenType::Identifier, current_token->type);
        KPL_ASSERT_THAT(!current_function_return_type_id.has_value());
        next_token(true); // eat type identifier
        const Token* identifier_token = current_token;
        previous_token(true); // Go back to type for parsing prototype
        std::unique_ptr<Prototype> prototype = parse_prototype();
        if (current_token->type == TokenType::End) {
            next_token(true); // eat 'end'
            if (prototype == nullptr) {
                return nullptr;
            }
            return std::make_unique<Function>(std::move(prototype),
                std::vector<std::unique_ptr<ASTNode>>{},
                linkage_type,
                identifier_token->source_location);
        }

        current_function_return_type_id = prototype->return_type_identifier_id;

        KPL_ASSERT_THAT(std::holds_alternative<StringId>(identifier_token->data));
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        const std::string_view identifier = StringPool::get().lookup(identifier_id);
        const std::string message = std::format("Function '{}' was not closed with an 'end' keyword", identifier);
        auto body = parse_body<TokenType::End>(message, identifier_token->source_location);
        next_token(true); // eat 'end' or 'EOF'

        current_function_return_type_id = std::nullopt;
        if (!prototype || !body) {
            return nullptr;
        }

        return std::make_unique<Function>(std::move(prototype), std::move(*body), linkage_type, identifier_token->source_location);
    }

}
