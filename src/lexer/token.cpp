// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "lexer/token.hpp"
#include "lexer/token_type.hpp"
#include "log.hpp"
#include "semantic_analysis/string_table.hpp"
#include <ostream>
#include <string>

namespace kepler::lexer {

    std::ostream& operator<<(std::ostream& os, Token token) {
        switch (token.type) {
            case TokenType::EndOfFile:
            case TokenType::Unknown:
            case TokenType::BracketOpen:
            case TokenType::BracketClose:
            case TokenType::Comma:
            case TokenType::Colon:
            case TokenType::Extern:
            case TokenType::Return:
            case TokenType::End:
            case TokenType::If:
            case TokenType::Else:
            case TokenType::Elseif:
            case TokenType::For:
            case TokenType::True:
            case TokenType::False: os << token.type; break;

            case TokenType::Operator:
                os << token.type << "(" << std::get<OperatorType>(token.data) << ")";
                break;
            case TokenType::Identifier: {
                semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(token.data);
                const std::string& identifier = semantic_analysis::StringTable::get().lookup(identifier_id);
                os << token.type << "(" << identifier << ")";
                break;
            }
            case TokenType::FloatingPointLiteral:
                os << token.type << "(" << std::get<double>(token.data) << ")";
                break;
            case TokenType::IntegerLiteral:
                os << token.type << "(" << std::get<int64_t>(token.data) << ")";
                break;
            case TokenType::StringLiteral: {
                semantic_analysis::StringId literal_id = std::get<semantic_analysis::StringId>(token.data);
                const std::string& literal = semantic_analysis::StringTable::get().lookup(literal_id);
                os << token.type << "(" << literal << ")";
                break;
            }
            case TokenType::DataType:
                os << token.type << "(" << std::get<type_system::DataTypeKind>(token.data) << ")";
                break;
            default:
                log(log_type::INTERNAL_LEXING_WARNING, "Missing implementation of operator '<<' for token of type'", (int)token.type, "'");
                break;
        }

        return os;
    }

}
