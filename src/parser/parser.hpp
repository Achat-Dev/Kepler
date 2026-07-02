// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/ast_node.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/statement.hpp"
#include "error_code.hpp"
#include "lexer/operator_type.hpp"
#include "lexer/token.hpp"
#include "semantic_analysis/string_table.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstddef>
#include <expected>
#include <memory>
#include <vector>

namespace kepler::parser {

    // TODO: Replace shared_ptr with raw pointers and an arena allocator once the architecture rework is finished
    class Parser {
    public:
        Parser(const std::vector<lexer::Token>& tokens)
            : tokens(tokens), current_token(&tokens[0]) {}
        std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, ErrorCode> parse();

    private:
        const std::vector<lexer::Token>& tokens;
        const lexer::Token* current_token;
        size_t current_token_index = 0;
        type_system::DataTypeKind current_parsing_function_return_type;

        int get_operator_precedence(lexer::OperatorType operator_type) const;
        void next_token();
        void previous_token();

        // Top level
        std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> parse_extern();
        std::expected<semantic_analysis::SymbolId, ErrorCode> parse_prototype(ast::Prototype::LinkageType linkage_type);
        std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> parse_top_level_data_type();
        std::expected<std::shared_ptr<ast::Function>, ErrorCode> parse_function(type_system::DataTypeKind return_type, semantic_analysis::StringId identifier_id);

        // Main body nodes
        std::expected<std::shared_ptr<ast::ASTNode>, ErrorCode> parse_statement();
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_expression();

        // Expressions
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_binary_expression_rhs(std::shared_ptr<ast::Expression> lhs_expression, int expression_precedence);
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_primary();
        std::shared_ptr<ast::Expression> parse_literal();
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_parenthesis();
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_identifier();
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_negative();
        std::expected<std::shared_ptr<ast::Expression>, ErrorCode> parse_cast();

        // Statements
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> parse_assignment();
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> parse_if();
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> parse_for();
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> create_for_statement(semantic_analysis::StringId variable_identifier_id, type_system::DataTypeKind variable_data_type, std::shared_ptr<ast::Expression> start_value, std::shared_ptr<ast::Expression> end_value, std::shared_ptr<ast::Expression> step_value);
        std::expected<std::vector<std::shared_ptr<ast::ASTNode>>, ErrorCode> parse_for_body();
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> parse_return();
        std::expected<std::shared_ptr<ast::Statement>, ErrorCode> parse_variable_declaration();
    };

}
