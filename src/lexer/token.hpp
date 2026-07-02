// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "lexer/operator_type.hpp"
#include "lexer/token_type.hpp"
#include "semantic_analysis/string_table.hpp"
#include "type_system/data_type_kind.hpp"
#include <cstdint>
#include <ostream>
#include <variant>

namespace kepler::lexer {

    struct Token {
        TokenType type;

        using TokenData = std::variant<double, // Floating point literals
            int64_t,                           // Integer literals
            semantic_analysis::StringId,       // String literals & identifiers
            bool,                              // Boolean literals
            OperatorType,                      // Operators
            type_system::DataTypeKind,         // Data types
            std::monostate>;
        TokenData data;
    };

    std::ostream& operator<<(std::ostream& os, Token token);

}
