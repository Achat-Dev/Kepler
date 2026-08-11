// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/operator_type.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <format>

namespace kepler {

    StringId get_operator_name_id(OperatorType operator_type) {
        switch (operator_type) {
            case OperatorType::Plus:
                return StringPool::get().store("__add");
            case OperatorType::Minus:
                return StringPool::get().store("__sub");
            case OperatorType::Multiplication:
                return StringPool::get().store("__mul");
            case OperatorType::Division:
                return StringPool::get().store("__div");
            case OperatorType::LessThan:
                return StringPool::get().store("__less_than");
            case OperatorType::GreaterThan:
                return StringPool::get().store("__greater_than");
            case OperatorType::Equals:
                return StringPool::get().store("__equals");
            case OperatorType::NotEquals:
                return StringPool::get().store("__not_equals");
            case OperatorType::LessEquals:
                return StringPool::get().store("__less_equals");
            case OperatorType::GreaterEquals:
                return StringPool::get().store("__greater_equals");
        }

        assert::unreachable(std::format("Missing operator name id implementation for operator type '{}'", static_cast<int>(operator_type)));
    }

}
