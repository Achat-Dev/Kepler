// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "io/file.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

namespace kepler::io {

    std::expected<File, diagnostics::Diagnostic> File::load(const std::string& path) {
        if (!std::filesystem::exists(path)) {
            return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::FileNotFound, std::format("File '{}' not found", path)));
        }
        if (std::filesystem::is_directory(path)) {
            return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::FileIsADirectory, std::format("Path '{}' is a directory", path)));
        }
        if (!std::filesystem::is_regular_file(path)) {
            return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::NotARegularFile, std::format("File '{}' is not a regular file", path)));
        }

        // Read file contents into string
        std::ifstream file_stream(path);
        if (!file_stream) {
            return std::unexpected(diagnostics::Diagnostic(diagnostics::DiagnosticCode::FailedToCreateFileStream, std::format("Check the permissions for '{}' and make sure that the file is not locked by other programs", path)));
        }

        const std::string content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        return File(path, std::move(content));
    }

}
