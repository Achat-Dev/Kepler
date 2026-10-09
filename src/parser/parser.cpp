// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "parser/parser.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/extern.hpp"
#include "ast/statements/import_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "ast/struct.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "utils/assert.h"
#include "utils/identifier_path.hpp"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <utility>
#include <vector>

namespace kepler {

    AbstractSyntaxTree Parser::parse() {
        KPL_ASSERT_THAT(!tokens.empty());
        KPL_ASSERT_THAT(tokens.back().type == TokenType::EndOfFile, "Final token should be EOF, received '{}'", tokens.back().type);

        AbstractSyntaxTree ast;
        while (current_token->type != TokenType::EndOfFile) {
            switch (current_token->type) {
                case TokenType::Module: {
                    std::unique_ptr<ModuleStatement> ast_node = parse_module();
                    if (ast_node) {
                        if (ast.module_statement != nullptr) {
                            diagnostic_sink.report(DiagnosticCode::ModuleRedefinition,
                                "'module' can only be specified once per file",
                                ast_node->source_location);
                            break;
                        }

                        ast.module_statement = std::move(ast_node);
                    }
                    break;
                }
                case TokenType::Import: {
                    std::unique_ptr<ImportStatement> ast_node = parse_import();
                    if (ast_node) {
                        ast.import_statements.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Export: {
                    std::unique_ptr<ExportableNode> ast_node = parse_export();
                    if (ast_node) {
                        if (ast_node->node_type == ASTNodeType::Struct) {
                            ast.struct_nodes.push_back(std::unique_ptr<Struct>(static_cast<Struct*>(ast_node.release())));
                        } else {
                            ast.top_level_nodes.push_back(std::move(ast_node));
                        }
                    }
                    break;
                }
                case TokenType::Extern: {
                    std::unique_ptr<Extern> ast_node = parse_extern(LinkageType::External);
                    if (ast_node) {
                        ast.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Struct: {
                    std::unique_ptr<Struct> ast_node = parse_struct(LinkageType::Internal);
                    if (ast_node) {
                        ast.struct_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Identifier: {
                    std::unique_ptr<ExportableNode> ast_node = parse_top_level_identifier(LinkageType::Internal);
                    if (ast_node) {
                        ast.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Newline:
                    next_token(true);
                    break;
                default:
                    diagnostic_sink.report(DiagnosticCode::UnexpectedToken,
                        std::format("Unexpected token '{}' on top level, expected '{}', '{}', '{}', '{}', '{}' or function definition",
                            current_token->type,
                            TokenType::Module,
                            TokenType::Import,
                            TokenType::Struct,
                            TokenType::Export,
                            TokenType::Extern),
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                    break;
            }
        }

        if (ast.module_statement == nullptr) {
            const StringId fallback_identifier_id = StringPool::get().store("__file://" + file->path.string());
            // clang-format off
            ast.module_statement = std::make_unique<ModuleStatement>(IdentifierPath{
                    .identifier_parts = {{.identifier_id = fallback_identifier_id, .source_location = {}}}
                },
                SourceLocation{});
            // clang-format on
        }

        return ast;
    }

    int Parser::get_operator_precedence(OperatorType operator_type) const {
        switch (operator_type) {
            case OperatorType::LessThan:
            case OperatorType::GreaterThan:
            case OperatorType::Equals:
            case OperatorType::NotEquals:
            case OperatorType::LessEquals:
            case OperatorType::GreaterEquals:
                return 10;

            case OperatorType::Plus:
            case OperatorType::Minus:
                return 20;

            case OperatorType::Multiplication:
            case OperatorType::Division:
                return 30;
        }

        KPL_ASSERT_UNREACHABLE("Missing binary operator precedence implementation for operator '{}'", static_cast<int>(operator_type));
    }

    void Parser::next_token(bool skip_newline) {
        if (current_token_index < tokens.size() - 1) {
            current_token_index++;
            current_token = &tokens[current_token_index];

            if (current_token->type == TokenType::Newline && skip_newline) {
                next_token(skip_newline);
            }
        }
    }

    void Parser::previous_token(bool skip_newline) {
        if (current_token_index > 0) {
            current_token_index--;
            current_token = &tokens[current_token_index];

            if (current_token->type == TokenType::Newline && skip_newline) {
                previous_token(skip_newline);
            }
        }
    }

    TokenType Parser::peek_next_token_type(size_t lookahead, bool skip_newline) const {
        KPL_ASSERT_THAT(lookahead > 0);
        TokenType result = TokenType::EndOfFile;
        size_t i = 1;
        while (i <= lookahead) {
            if (current_token_index + i >= tokens.size()) {
                return TokenType::EndOfFile;
            }

            result = tokens[current_token_index + i].type;
            if (result == TokenType::Newline && skip_newline) {
                lookahead++;
            }
            i++;
        }
        return result;
    }

    void Parser::jump_to_token(size_t index) {
        KPL_ASSERT_THAT(index < tokens.size());
        current_token_index = index;
        current_token = &tokens[current_token_index];
    }

    // TODO (improvement): Maybe don't allow newlines between submodule identifiers
    // clang-format off
    std::optional<IdentifierPathParseResult> Parser::parse_identifier_path(TokenType delimiter_token,
        IdentifierPathParseKind path_return_kind,
        const std::string& diagnostic_ending)
    {
        // clang-format on
        KPL_ASSERT_THAT(current_token->type == TokenType::Identifier);
        KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
        const uint32_t source_location_start_position = current_token->source_location.position;
        std::vector<IdentifierPathPart> identifier_parts{
            {std::get<StringId>(current_token->data), current_token->source_location},
        };
        next_token(true); // eat identifier
        while (current_token->type == delimiter_token) {
            next_token(true); // eat delimiter
            if (current_token->type != TokenType::Identifier) {
                previous_token(true); // jump back to delimiter because otherwise the next line will be skipped because of the recovery
                const std::string message = std::format("Expected identifier after '{}' {}", delimiter_token, diagnostic_ending);
                diagnostic_sink.report(DiagnosticCode::UnexpectedToken, std::move(message), current_token->source_location);
                recover(SynchronizationSet<TokenType::Newline, TokenType::End>{}, SynchronizationSet<TokenType::Newline>{});
                return std::nullopt;
            }
            KPL_ASSERT_THAT(std::holds_alternative<StringId>(current_token->data));
            identifier_parts.push_back({
                .identifier_id = std::get<StringId>(current_token->data),
                .source_location = current_token->source_location,
            });
            next_token(true); // eat identifier
        }

        previous_token(true); // Go back to last identifier
        // TODO (improvement): This code is ugly
        switch (path_return_kind) {
            case IdentifierPathParseKind::IncludeLastIdentifier: {
                const SourceLocation source_location{
                    .file_id = file->id,
                    .position = source_location_start_position,
                    .size = (current_token->source_location.position + current_token->source_location.size) - source_location_start_position,
                };
                next_token(true); // eat last identifier
                return IdentifierPathParseResult{
                    .identifier_path = {
                        .identifier_parts = std::move(identifier_parts),
                        .separator_id = StringPool::get().store(std::format("{}", delimiter_token)),
                        .source_location = std::move(source_location),
                    },
                };
            }
            case IdentifierPathParseKind::ReturnLastIdentifierSeparately: {
                identifier_parts.pop_back();
                previous_token(true); // Go back to last delimiter
                previous_token(true); // Go back to last actual identifier
                const SourceLocation source_location{
                    .file_id = file->id,
                    .position = source_location_start_position,
                    .size = (current_token->source_location.position + current_token->source_location.size) - source_location_start_position,
                };
                next_token(true); // eat last actual identifier
                next_token(true); // eat last delimiter
                const Token* last_identifier_token = current_token;
                next_token(true); // eat last identifier
                KPL_ASSERT_THAT(std::holds_alternative<StringId>(last_identifier_token->data));
                return IdentifierPathParseResult{
                    .identifier_path = {
                        .identifier_parts = std::move(identifier_parts),
                        .separator_id = StringPool::get().store(std::format("{}", delimiter_token)),
                        .source_location = std::move(source_location),
                    },
                    .last_identifier = std::get<StringId>(last_identifier_token->data),
                    .last_identifier_source_location = last_identifier_token->source_location,
                };
            }
            case IdentifierPathParseKind::ReturnAtLastIdentifier: {
                identifier_parts.pop_back();
                previous_token(true); // Go back to last delimiter
                previous_token(true); // Go back to last actual identifier
                const SourceLocation source_location{
                    .file_id = file->id,
                    .position = source_location_start_position,
                    .size = (current_token->source_location.position + current_token->source_location.size) - source_location_start_position,
                };
                next_token(true); // eat last actual identifier
                next_token(true); // eat last delimiter
                return IdentifierPathParseResult{
                    .identifier_path = {
                        .identifier_parts = std::move(identifier_parts),
                        .separator_id = StringPool::get().store(std::format("{}", delimiter_token)),
                        .source_location = std::move(source_location),
                    },
                };
            }
        }
        KPL_ASSERT_UNREACHABLE("Missing identifier path parse implementation for return kind '{}'", static_cast<int>(path_return_kind));
    }

    IdentifierPath Parser::identifier_id_to_path(StringId identifier_id, SourceLocation source_location) const {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        return {
            .identifier_parts = {{
                .identifier_id = identifier_id,
                .source_location = source_location,
            }},
            .source_location = source_location,
        };
    }

}
