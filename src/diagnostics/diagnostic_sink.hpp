// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_severity.hpp"
#include "diagnostics/source_location.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace kepler {

    class DiagnosticSink {
    public:
        void report(DiagnosticCode code, std::string message);
        void report(DiagnosticCode code, std::string message, SourceLocation source_location);
        void flush();
        uint32_t get_warning_count() const { return warning_count; }
        uint32_t get_error_count() const { return error_count; }

    private:
        uint32_t warning_count = 0;
        uint32_t error_count = 0;
        std::vector<SourceDiagnostic> diagnostics;

        size_t strlen_utf8(const std::string& string) const;
        std::string get_severity_highlight(DiagnosticSeverity severity) const;
    };

}
