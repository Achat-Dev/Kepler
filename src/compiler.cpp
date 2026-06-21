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
#include "log.hpp"

namespace Kepler {

    void Compiler::compile_project() const {
        Lexer::Tokenizer tokenizer(arguments.input_file_name);
        auto tokens = tokenizer.tokenize();
        if (!tokens) {
            log(LogType::COMPILE_ERROR, "Failed to compile file '", arguments.input_file_name, "'");
            exit((int)tokens.error());
        }
    }

}
