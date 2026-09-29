// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "utils/string_pool.hpp"
#include <string>
#include <vector>

namespace kepler {

    struct IdentifierPathPart {
        StringId identifier_id;
        SourceLocation source_location;

        bool operator==(const IdentifierPathPart& other) const;
        bool operator!=(const IdentifierPathPart& other) const;
    };

    struct IdentifierPath {
        std::vector<IdentifierPathPart> identifier_parts;
        StringId separator_id;
        SourceLocation source_location;

        bool operator==(const IdentifierPath& other) const;
        bool operator!=(const IdentifierPath& other) const;
    };

    std::string get_full_identifier_from_path(const IdentifierPath& identifier_path);

}
