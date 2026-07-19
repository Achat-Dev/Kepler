// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compilation_context.hpp"
#include "compiler.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_severity.hpp"
#include <cstdlib>
#include <print>

int main(int argc, char* argv[]) {
    const auto context = kepler::parse_command_line_arguments(argc, argv);

    if (!context) {
        const kepler::Diagnostic diagnostic = context.error();

        if (diagnostic.code == kepler::DiagnosticCode::HelpRequested) {
            std::println("{}", diagnostic.message);
            return EXIT_SUCCESS;
        } else {
            std::println("{}{}", kepler::get_diagnostic_severity(diagnostic.code), diagnostic.message);
            return EXIT_FAILURE;
        }
    }

    kepler::compile_project(*context);
}
