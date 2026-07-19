// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/source_location.hpp"
#include <string>

namespace kepler {

    struct Diagnostic {
        DiagnosticCode code;
        std::string message;
    };

    struct SourceDiagnostic {
        DiagnosticCode code;
        std::string message;
        SourceLocation source_location;
    };

}
