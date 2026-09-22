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
#include "ast/statements/import_statement.hpp"
#include "ast/statements/module_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "lexer/token.hpp"
#include "semantic_analysis/module.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <utility>
#include <vector>

namespace kepler {

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

    void Parser::jump_to_token(size_t index) {
        KPL_ASSERT_THAT(index < tokens.size());
        current_token_index = index;
        current_token = &tokens[current_token_index];
    }

    AbstractSyntaxTree Parser::parse() {
        KPL_ASSERT_THAT(!tokens.empty());
        KPL_ASSERT_THAT(tokens.back().type == TokenType::EndOfFile, "Final token should be EOF, received '{}'", tokens.back().type);

        AbstractSyntaxTree ast;
        while (current_token->type != TokenType::EndOfFile) {
            switch (current_token->type) {
                case TokenType::Module: {
                    std::unique_ptr<ModuleStatement> ast_node = parse_module();
                    if (ast_node) {
                        KPL_ASSERT_THAT(!ast_node->module_path.part_identifier_ids.empty());
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
                    std::unique_ptr<ASTNode> ast_node = parse_export();
                    if (ast_node) {
                        ast.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Extern: {
                    std::unique_ptr<ASTNode> ast_node = parse_extern(LinkageType::External);
                    if (ast_node) {
                        ast.top_level_nodes.push_back(std::move(ast_node));
                    }
                    break;
                }
                case TokenType::Type: {
                    std::unique_ptr<ASTNode> ast_node = parse_top_level_type(LinkageType::Internal);
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
                        std::format("Unexpected token '{}' on top level, expected 'extern' or function definition", current_token->type),
                        current_token->source_location);
                    recover(SynchronizationSet<TokenType::Newline>{}, SynchronizationSet<TokenType::Newline>{});
                    break;
            }
        }

        if (ast.module_statement == nullptr) {
            const StringId fallback_identifier_id = StringPool::get().store("__file://" + file->path.string());
            ast.module_statement = std::make_unique<ModuleStatement>(ModulePath{.part_identifier_ids = {fallback_identifier_id}},
                SourceLocation{});
        }

        return ast;
    }

}
