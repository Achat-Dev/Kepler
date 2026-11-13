// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/operator_kind.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"

namespace kepler {

    StringId get_operator_identifier_id(OperatorKind operator_kind) {
        switch (operator_kind) {
            case OperatorKind::Plus:
                return StringPool::get().store("__add");
            case OperatorKind::Minus:
                return StringPool::get().store("__sub");
            case OperatorKind::Multiplication:
                return StringPool::get().store("__mul");
            case OperatorKind::Division:
                return StringPool::get().store("__div");
            case OperatorKind::LessThan:
                return StringPool::get().store("__less_than");
            case OperatorKind::GreaterThan:
                return StringPool::get().store("__greater_than");
            case OperatorKind::Equals:
                return StringPool::get().store("__equals");
            case OperatorKind::NotEquals:
                return StringPool::get().store("__not_equals");
            case OperatorKind::LessEquals:
                return StringPool::get().store("__less_equals");
            case OperatorKind::GreaterEquals:
                return StringPool::get().store("__greater_equals");
        }

        KPL_ASSERT_UNREACHABLE("Missing operator name id implementation for operator kind '{}'", static_cast<int>(operator_kind));
    }

}
