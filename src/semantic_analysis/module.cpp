// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/module.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <string>

namespace kepler {

    std::string get_full_module_identifier(const ModulePath& module_path) {
        KPL_ASSERT_THAT(!module_path.part_identifier_ids.empty());
        std::string result;
        for (size_t i = 0; i < module_path.part_identifier_ids.size(); i++) {
            KPL_ASSERT_THAT(module_path.part_identifier_ids[i] != StringId::invalid());
            result += StringPool::get().lookup(module_path.part_identifier_ids[i]);
            if (i < module_path.part_identifier_ids.size() - 1) {
                result += "::";
            }
        }
        return result;
    }

}
