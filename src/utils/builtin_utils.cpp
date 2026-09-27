// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/builtin_utils.hpp"
#include "diagnostics/diagnostic.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <expected>
#include <string>
#include <utility>

namespace kepler {

    std::expected<void, Diagnostic> is_builtin_type_identifier(StringId identifier_id, const std::string& usage_message) {
        KPL_ASSERT_THAT(identifier_id != StringId::invalid());
        if (builtin_type_identifier.contains(identifier_id)) {
            const std::string_view identifier = StringPool::get().lookup(identifier_id);
            const std::string message = std::format("'{}' is a reserved keyword and cannot be used {}", identifier, usage_message);
            return std::unexpected(Diagnostic{.code = DiagnosticCode::InvalidIdentifier, .message = std::move(message)});
        }
        return {};
    }

}
