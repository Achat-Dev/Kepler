// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic_code.hpp"
#include <expected>
#include <string>
#include <utility>

namespace kepler::io {

    struct FileError {
        diagnostics::DiagnosticCode code;
        std::string message;
    };

    class File {
    public:
        const std::string path;
        const std::string content;

        static std::expected<File, FileError> load(const std::string& path);

    private:
        File(std::string path, std::string content)
            : path(std::move(path)), content(std::move(content)) {}
    };

}
