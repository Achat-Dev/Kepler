// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/scope.hpp"
#include "utils/string_pool.hpp"
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct ModuleId {
        uint32_t value = std::numeric_limits<uint32_t>::max();

        bool operator==(const ModuleId& other) const = default;
        bool operator!=(const ModuleId& other) const = default;
        static constexpr ModuleId invalid() { return ModuleId{}; }
    };

    struct ModulePath {
        std::vector<StringId> part_identifier_ids;

        bool operator==(const ModulePath& other) const = default;
        bool operator!=(const ModulePath& other) const = default;
    };

    struct Module {
        ModuleId id;
        StringId identifier_id;
        StringId full_identifier_id;
        std::vector<ScopeId> scope_ids;
        std::vector<ModuleId> imported_module_ids;
        std::unordered_map<StringId, ModuleId> submodule_ids;
        ScopeId current_scope_id;
    };

    std::string get_full_module_identifier(const ModulePath& module_path);

}

template <>
struct std::hash<kepler::ModuleId> {
    size_t operator()(const kepler::ModuleId& id) const noexcept {
        return hash<uint32_t>{}(id.value);
    }
};
