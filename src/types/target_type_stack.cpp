// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/target_type_stack.hpp"

#include "types/type_token.hpp"

#include <cassert>
#include <stack>

namespace Kepler::Type::TargetTypeStack {

    static std::stack<Type::TypeToken> target_types;

    void push(Type::TypeToken type) {
        target_types.push(type);
    }

    void pop() {
        return target_types.pop();
    }

    Type::TypeToken top() {
        assert(!target_types.empty() && "[ Assertion ]: peeking at empty TargetTypeStack");
        return target_types.top();
    }

}
