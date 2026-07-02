// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/operator_type.hpp"
#include "log.hpp"
#include <ostream>

namespace kepler::lexer {

    std::ostream& operator<<(std::ostream& os, OperatorType operator_type) {
        switch (operator_type) {
            case OperatorType::Plus: os << '+'; break;
            case OperatorType::Minus: os << '-'; break;
            case OperatorType::Multiplication: os << '*'; break;
            case OperatorType::Division: os << '/'; break;
            case OperatorType::LessThan: os << '<'; break;
            case OperatorType::GreaterThan: os << '>'; break;
            case OperatorType::Equals: os << "=="; break;
            case OperatorType::NotEquals: os << "!="; break;
            case OperatorType::LessEquals: os << "<="; break;
            case OperatorType::GreaterEquals: os << ">="; break;
            default: log(log_type::INTERNAL_LEXING_WARNING, "Missing implementation of operator '<<' for operator type '", (int)operator_type, '\''); break;
        }

        return os;
    }

}
