// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"
#include "lexer/tokenizer.hpp"
#include "parser/parser.hpp"

namespace kepler {

    void Compiler::compile_project() const {
        lexer::Tokenizer tokenizer(arguments.input_file_name);
        const auto tokens = tokenizer.tokenize();
        if (!tokens) {
            exit(static_cast<int>(tokens.error()));
        }

        parser::Parser parser(arguments.input_file_name, *tokens);
        const auto ast_nodes = parser.parse();
        if (!ast_nodes) {
            exit(static_cast<int>(tokens.error()));
        }
    }

}
