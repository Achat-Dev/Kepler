// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/type_token.hpp"

#include "types/type.hpp"

#include <ostream>

namespace Kepler::Type {

    std::ostream& operator<<(std::ostream& os, TypeToken type) {
        os << get_type_name(type);
        return os;
    }

}
