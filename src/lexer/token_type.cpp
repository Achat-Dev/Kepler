// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/token_type.hpp"
#include "log.hpp"
#include <ostream>

namespace kepler::lexer {

    std::ostream& operator<<(std::ostream& os, TokenType type) {
        switch (type) {
            case TokenType::EndOfFile: os << "EOF"; break;
            case TokenType::Unknown: os << "unknown character"; break;
            case TokenType::BracketOpen: os << '('; break;
            case TokenType::BracketClose: os << ')'; break;
            case TokenType::Comma: os << ','; break;
            case TokenType::Colon: os << ':'; break;
            case TokenType::Extern: os << "extern"; break;
            case TokenType::Return: os << "return"; break;
            case TokenType::End: os << "end"; break;
            case TokenType::If: os << "if"; break;
            case TokenType::Else: os << "else"; break;
            case TokenType::Elseif: os << "elseif"; break;
            case TokenType::For: os << "for"; break;
            case TokenType::True: os << "true"; break;
            case TokenType::False: os << "false"; break;
            case TokenType::Operator: os << "operator("; break;
            case TokenType::Identifier: os << "identifier("; break;
            case TokenType::FloatingPointLiteral: os << "floating point literal("; break;
            case TokenType::IntegerLiteral: os << "integer literal("; break;
            case TokenType::StringLiteral: os << "string literal("; break;
            case TokenType::DataType: os << "data type("; break;
            default: log(log_type::LEXING_WARNING, "Missing implementation of operator '<<' for token type'", (int)type, "'"); break;
        }

        return os;
    }

}