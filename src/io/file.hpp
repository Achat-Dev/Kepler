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
#include "io/file_id.hpp"
#include <expected>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace kepler::io {

    struct File {
        FileId id;
        std::string content;

        static std::expected<const File, diagnostics::Diagnostic> load(const std::filesystem::path& path);
        static const std::filesystem::path* get_path_by_id(FileId id);

    private:
        File(FileId id, std::string content)
            : id(id), content(std::move(content)) {}

        inline static std::vector<std::filesystem::path> known_paths{};
    };

}
