// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/type_token.hpp"

#include <string>

namespace Kepler::AST {

    struct ParameterData {
        const Type::TypeToken type;
        const std::string name;

        ParameterData(Type::TypeToken type, std::string name) : type(type), name(std::move(name)) {}
    };

}
