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
#include "utils/assert.h"
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

        const std::string content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        const auto it = std::find(known_paths.begin(), known_paths.end(), path);
        if (it == known_paths.end()) {
            uint32_t known_path_count = known_paths.size();
            known_paths.push_back(path);
            return File({.value = known_path_count}, std::move(content));
        } else {
            const uint32_t path_index = std::distance(known_paths.begin(), it);
            return File({.value = path_index}, std::move(content));
        }
    }

    std::filesystem::path File::get_path_by_id(FileId id) {
        KPL_ASSERT_THAT(id.value < known_paths.size(), "File path with id '{}' doesn't exist", id);
        return known_paths[id.value];
    }

}
