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
#include "utils/string_pool.hpp"
#include <expected>
#include <string>
#include <unordered_set>

namespace kepler {

    inline const std::unordered_set<StringId> builtin_type_identifier{
        StringPool::get().store("void"),
        StringPool::get().store("bool"),
        StringPool::get().store("string"),
        StringPool::get().store("i8"),
        StringPool::get().store("i16"),
        StringPool::get().store("i32"),
        StringPool::get().store("i64"),
        StringPool::get().store("u8"),
        StringPool::get().store("u16"),
        StringPool::get().store("u32"),
        StringPool::get().store("u64"),
        StringPool::get().store("f32"),
        StringPool::get().store("f64"),
    };

    std::expected<void, Diagnostic> is_builtin_type_identifier(StringId identifier_id, const std::string& usage_message);

}
