// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compilation_context.hpp"
#include "compiler.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include <cstdlib>
#include <print>

int main(int argc, char* argv[]) {
    const auto context = kepler::parse_command_line_arguments(argc, argv);

    if (!context) {
        const kepler::UsageError error = context.error();
        
        if (error.code == kepler::diagnostics::DiagnosticCode::HelpRequested) {
            std::println("{}", error.message);
            return EXIT_SUCCESS;
        } else {
            std::println("{}{}", kepler::diagnostics::get_severity(error.code), error.message);
            return EXIT_FAILURE;
        }
    }

    kepler::compile_project(*context);
}
