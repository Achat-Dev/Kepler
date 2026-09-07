// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/diagnostic.hpp"
#include "io/file.hpp"
#include <expected>
#include <filesystem>
#include <unordered_map>

namespace kepler {

    class FileManager {
    public:
        std::expected<File*, Diagnostic> load(const std::filesystem::path& path);
        File* lookup(FileId id);

        static FileManager& get();

    private:
        std::unordered_map<FileId, std::filesystem::path> id_to_path_map;
        std::unordered_map<std::filesystem::path, File> files;

        FileManager() = default;
        ~FileManager() = default;
        FileManager(const FileManager& other) = delete;
        FileManager& operator=(const FileManager&) = delete;
        FileManager(FileManager&&) = delete;
        FileManager& operator=(FileManager&&) = delete;
    };

}
