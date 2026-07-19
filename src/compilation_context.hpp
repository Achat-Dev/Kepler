// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include <expected>
#include <string>

namespace kepler {

    struct CompilationContext {
        std::string input_file_path;
        std::string output_file_path;
        bool log_verbose = false;
    };

    std::expected<CompilationContext, Diagnostic> parse_command_line_arguments(int argc, char* argv[]);

}
