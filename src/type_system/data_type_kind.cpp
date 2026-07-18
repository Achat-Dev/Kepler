// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "type_system/data_type_kind.hpp"

namespace kepler {

    bool is_integer_type(DataTypeKind data_type) {
        return data_type == DataTypeKind::Int8 || data_type == DataTypeKind::Int16 || data_type == DataTypeKind::Int32 || data_type == DataTypeKind::Int64;
    }

}
