// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "io/file.hpp"
#include "diagnostics/diagnostic.hpp"
#include "log.hpp"
#include <algorithm>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

namespace kepler {

    std::expected<const File, Diagnostic> File::load(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FileNotFound,
                .message = std::format("File '{}' not found", path.c_str()),
            });
        }
        if (std::filesystem::is_directory(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FileIsADirectory,
                .message = std::format("Path '{}' is a directory", path.c_str()),
            });
        }
        if (!std::filesystem::is_regular_file(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::NotARegularFile,
                .message = std::format("File '{}' is not a regular file", path.c_str()),
            });
        }

        // Read file contents into string
        std::ifstream file_stream(path);
        if (!file_stream) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FailedToCreateFileStream,
                .message = std::format("Check the permissions for '{}' and make sure that the file is not locked by other programs", path.c_str()),
            });
        }

        uint32_t known_path_count = known_paths.size();
        if (!std::ranges::contains(known_paths, path)) {
            known_paths.push_back(path);
        }
        const std::string content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        return File({known_path_count}, std::move(content));
    }

    const std::filesystem::path* File::get_path_by_id(FileId id) {
        if (id.value >= known_paths.size()) {
            log::error("File path with id '{}' doesn't exist", id);
            return nullptr;
        }
        return &known_paths[id.value];
    }

}
