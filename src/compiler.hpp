// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "diagnostics/diagnostic.hpp"
#include <expected>
#include <filesystem>
#include <string>

namespace kepler {

    struct CompilerContext {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        bool log_verbose;
        bool help_requested;
        std::string help;
    };

    class Compiler {
    public:
        int run(int argc, char** argv) const;

    private:
        void verify_ast(const AbstractSyntaxTree& ast) const;
        std::expected<CompilerContext, Diagnostic> parse_args(int argc, char** argv) const;
    };

}
