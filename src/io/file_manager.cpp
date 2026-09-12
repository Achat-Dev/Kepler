// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "io/file_manager.hpp"
#include "diagnostics/diagnostic.hpp"
#include "io/file.hpp"
#include "utils/assert.h"
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

namespace kepler {

    std::expected<File*, Diagnostic> FileManager::load(const std::filesystem::path& path) {
        if (files.contains(path)) {
            return &files[path];
        }

        if (!std::filesystem::exists(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FileNotFound,
                .message = std::format("File '{}' not found", path.string()),
            });
        }
        if (std::filesystem::is_directory(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FileIsADirectory,
                .message = std::format("Path '{}' is a directory", path.string()),
            });
        }
        if (!std::filesystem::is_regular_file(path)) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::NotARegularFile,
                .message = std::format("File '{}' is not a regular file", path.string()),
            });
        }

        // Read file content into string stream
        std::ifstream file_stream(path);
        if (!file_stream) {
            return std::unexpected(Diagnostic{
                .code = DiagnosticCode::FailedToCreateFileStream,
                .message = std::format("Check the permissions for '{}' and make sure that the file is not locked by other programs", path.string()),
            });
        }

        File file{
            .id = {.value = static_cast<uint32_t>(files.size())},
            .path = path,
            .content = std::string((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>()),
        };
        id_to_path_map.emplace(file.id, path);
        const auto [it, emplaced] = files.emplace(path, std::move(file));
        KPL_ASSERT_THAT(emplaced);
        KPL_ASSERT_THAT(id_to_path_map.size() == files.size(), "Id count: {}, file count: {}", id_to_path_map.size(), files.size());
        return &it->second;
    }

    File* FileManager::lookup(FileId id) {
        KPL_ASSERT_THAT(id.value < id_to_path_map.size(), "Looking up out of bounds FileId; file count is {}, received id {}", id_to_path_map.size(), id);
        const std::filesystem::path path = id_to_path_map[id];
        return &files[path];
    }

    FileManager& FileManager::get() {
        static FileManager instance;
        return instance;
    }

}
