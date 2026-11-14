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
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace kepler {

    ModuleStatement* Parser::parse_module() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Module,
            "Required token: '{}', received: '{}'",
            TokenKind::Module,
            current_token->token_kind);
        const SourceLocation& module_source_location = current_token->source_location;
        next_token(true); // eat 'module' keyword
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'module'", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        const auto module_path_parse_result = parse_identifier_path(TokenKind::DoubleColon,
            IdentifierPathParseKind::IncludeLastIdentifier,
            "in module path");
        if (!module_path_parse_result.has_value()) {
            return nullptr;
        }
        return allocator->allocate<ModuleStatement>(std::move(module_path_parse_result.value()), module_source_location);
    }

    ImportStatement* Parser::parse_import() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Import,
            "Required token: '{}', received: '{}'",
            TokenKind::Import,
            current_token->token_kind);
        const SourceLocation& import_source_location = current_token->source_location;
        next_token(true); // eat 'import' keyword
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'import'", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        const auto module_path_parse_result = parse_identifier_path(TokenKind::DoubleColon,
            IdentifierPathParseKind::IncludeLastIdentifier,
            "in imported module path");
        if (!module_path_parse_result.has_value()) {
            return nullptr;
        }
        return allocator->allocate<ImportStatement>(std::move(module_path_parse_result.value()), import_source_location);
    }

    ExportableNode* Parser::parse_export() {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Export,
            "Required token: '{}', received: '{}'",
            TokenKind::Export,
            current_token->token_kind);
        next_token(true); // eat 'export'
        switch (current_token->token_kind) {
            case TokenKind::Extern:
                return parse_extern(LinkageKind::Export);
                break;
            case TokenKind::Struct:
                return parse_struct(LinkageKind::Export);
                break;
            case TokenKind::Identifier:
                return parse_top_level_identifier(LinkageKind::Export);
                break;
            default:
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected 'extern' or type after 'export'", current_token->source_location);
                recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
        }
    }

    Extern* Parser::parse_extern(LinkageKind linkage_kind) {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Extern,
            "Required token: '{}', received: '{}'",
            TokenKind::Extern,
            current_token->token_kind);
        const SourceLocation& extern_source_location = current_token->source_location;
        next_token(true); // eat 'extern' keyword
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type after 'extern'", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        Prototype* prototype = parse_prototype();
        if (!prototype) {
            return nullptr;
        }
        return allocator->allocate<Extern>(prototype, linkage_kind, extern_source_location);
    }

    Prototype* Parser::parse_prototype() {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId return_type_identifier_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type identifier
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after return type of prototype", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        PrototypeIdentifier prototype_identifier = std::get<StringId>(current_token->data);
        SourceLocation identifier_source_location = current_token->source_location;
        const Token* identifier_token = current_token;

        next_token(true); // eat identifier
        if (current_token->token_kind == TokenKind::Dot) {
            // Don't use parse_identifier_path because a prototype path must be exactly two parts long (Type identifier + function identifier)
            next_token(true); // eat '.'
            if (current_token->token_kind != TokenKind::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after '.' in prototype identifier",
                    current_token->source_location);
                recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
            }
            identifier_source_location = {
                .file_id = current_token->source_location.file_id,
                .position = identifier_token->source_location.position,
                .size = (current_token->source_location.position + current_token->source_location.size) - identifier_token->source_location.position,
            };
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            prototype_identifier = MemberFunctionIdentifier{
                .type_identifier_id = std::get<StringId>(identifier_token->data),
                .identifier_id = std::get<StringId>(current_token->data),
                .type_source_location = identifier_token->source_location,
                .identifier_source_location = current_token->source_location,
            };
            next_token(true); // eat second identifier
        }
        if (current_token->token_kind != TokenKind::BracketOpen) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after prototype identifier", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        next_token(true); // eat '('
                          // clang-format off
        if (current_token->token_kind != TokenKind::Identifier
            && current_token->token_kind != TokenKind::Variadic
            && current_token->token_kind != TokenKind::BracketClose)
        {
            // clang-format on
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type or ')' after '(' in prototype", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        // Parse parameters
        std::vector<ParameterData> parameter_data;
        bool is_variadic = false;
        while (current_token->token_kind == TokenKind::Identifier) {
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            const Token* parameter_type_token = current_token;
            next_token(true); // eat type identifier
            if (current_token->token_kind != TokenKind::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after parameter type", current_token->source_location);
                recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                return nullptr;
            }

            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            parameter_data.push_back({
                .type_identifier_id = std::get<StringId>(parameter_type_token->data),
                .identifier_id = std::get<StringId>(current_token->data),
                .type_source_location = parameter_type_token->source_location,
                .identifier_source_location = current_token->source_location,
            });

            next_token(true); // eat identifier
            if (current_token->token_kind == TokenKind::Comma) {
                next_token(true); // eat ','
                // Lookahead for correct diagnostic
                if (current_token->token_kind != TokenKind::Identifier && current_token->token_kind != TokenKind::Variadic) {
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        "Expected type after ',' in prototype parameters",
                        current_token->source_location);
                    recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
                    return nullptr;
                }
            }
        }

        // Check for variadic function
        if (current_token->token_kind == TokenKind::Variadic) {
            next_token(true); // eat '...'
            is_variadic = true;
        }

        if (current_token->token_kind != TokenKind::BracketClose) {
            std::string message;
            if (is_variadic) {
                message = "Expected ')' after variadic parameter in prototype (variadic must be the last parameter)";
            } else {
                message = "Expected ')' after function parameters in prototype";
            }
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::move(message), current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }
        next_token(true); // eat ')'

        return allocator->allocate<Prototype>(return_type_identifier_id,
            std::move(prototype_identifier),
            std::move(parameter_data),
            is_variadic,
            type_source_location,
            std::move(identifier_source_location));
    }

    Struct* Parser::parse_struct(LinkageKind linkage_kind) {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Struct);
        const SourceLocation& struct_source_location = current_token->source_location;
        next_token(true); // eat 'struct' keyword
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after 'struct'", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline, TokenKind::End>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const Token* identifier_token = current_token;
        next_token(true); // eat identifier

        std::vector<StructNodeMember> members;
        while (current_token->token_kind != TokenKind::End) {
            if (current_token->token_kind == TokenKind::EndOfFile) {
                const StringId identifier_id = std::get<StringId>(identifier_token->data);
                const std::string message = std::format("struct '{}' was not closed with an 'end' keyword", StringPool::get().lookup(identifier_id));
                diagnostic_sink.report(DiagnosticCode::MissingEndKeyword, std::move(message), struct_source_location);
                return nullptr;
            }

            if (current_token->token_kind != TokenKind::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected type for struct member", current_token->source_location);
                recover(SynchronizationSet<TokenKind::End>{}, SynchronizationSet<>{});
                continue;
            }
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            const Token* member_type_token = current_token;

            next_token(true); // eat type identifier
            if (current_token->token_kind != TokenKind::Identifier) {
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after type for struct member", current_token->source_location);
                recover(SynchronizationSet<TokenKind::End>{}, SynchronizationSet<>{});
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
        return allocator->allocate<Struct>(std::get<StringId>(identifier_token->data),
            std::move(members),
            linkage_kind,
            identifier_token->source_location);
    }

    ExportableNode* Parser::parse_top_level_identifier(LinkageKind linkage_kind) {
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const StringId type_identifier_id = std::get<StringId>(current_token->data);
        const SourceLocation& type_source_location = current_token->source_location;
        next_token(true); // eat type identifier
        if (current_token->token_kind != TokenKind::Identifier) {
            diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected identifier after type on top level", current_token->source_location);
            recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        }

        next_token(true); // eat identifier
        // Variable definition
        if (current_token->token_kind == TokenKind::Assignment) {
            diagnostic_sink.report(DiagnosticCode::Unsupported, "Global variables are not supported yet", type_source_location);
            recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
            return nullptr;
        } else if (current_token->token_kind == TokenKind::Dot || current_token->token_kind == TokenKind::BracketOpen) {
            previous_token(true); // Go back to identifier
            previous_token(true); // Go back to type
            return parse_function(linkage_kind);
        }

        diagnostic_sink.report(DiagnosticCode::UnexpectedToken, "Expected '(' after identifier on top level", current_token->source_location);
        recover(SynchronizationSet<TokenKind::Newline>{}, SynchronizationSet<TokenKind::Newline>{});
        return nullptr;
    }

    Function* Parser::parse_function(LinkageKind linkage_kind) {
        KPL_ASSERT_NOT_NULLPTR(allocator);
        KPL_ASSERT_NOT_NULLPTR(current_token);
        KPL_ASSERT_THAT(current_token->token_kind == TokenKind::Identifier,
            "Required token: '{}', received: '{}'",
            TokenKind::Identifier,
            current_token->token_kind);
        next_token(true); // eat type identifier
        const Token* identifier_token = current_token;
        previous_token(true); // Go back to type for parsing prototype
        Prototype* prototype = parse_prototype();
        if (current_token->token_kind == TokenKind::End) {
            next_token(true); // eat 'end'
            if (prototype == nullptr) {
                return nullptr;
            }
            return allocator->allocate<Function>(prototype,
                std::vector<ASTNode*>{},
                linkage_kind,
                identifier_token->source_location);
        }

        KPL_ASSERT_THAT(std::holds_alternative<StringId>(identifier_token->data));
        const StringId identifier_id = std::get<StringId>(identifier_token->data);
        const std::string& identifier = StringPool::get().lookup(identifier_id);
        const std::string message = std::format("Function '{}' was not closed with an 'end' keyword", identifier);
        const auto body = parse_body<TokenKind::End>(message, identifier_token->source_location);
        next_token(true); // eat 'end' or 'EOF'

        if (prototype == nullptr || !body.has_value()) {
            return nullptr;
        }
        return allocator->allocate<Function>(prototype, std::move(body.value()), linkage_kind, identifier_token->source_location);
    }

}
