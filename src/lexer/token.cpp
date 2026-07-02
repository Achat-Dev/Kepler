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
#include <cstdint>
#include <ios>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>

namespace kepler::lexer {

    std::ostream& operator<<(std::ostream& os, Token token) {
        switch (token.type) {
            case TokenType::EndOfFile:
            case TokenType::Unknown:
            case TokenType::BracketOpen:
            case TokenType::BracketClose:
            case TokenType::Comma:
            case TokenType::Colon:
            case TokenType::Assignment:
            case TokenType::Extern:
            case TokenType::Return:
            case TokenType::End:
            case TokenType::If:
            case TokenType::Else:
            case TokenType::Elseif:
            case TokenType::For:
                os << token.type;
                break;

            case TokenType::Operator:
                os << token.type << "(" << std::get<OperatorType>(token.data) << ")";
                break;
            case TokenType::Identifier: {
                semantic_analysis::StringId identifier_id = std::get<semantic_analysis::StringId>(token.data);
                const std::string& identifier = semantic_analysis::StringTable::get().lookup(identifier_id);
                os << token.type << "(" << identifier << ")";
                break;
            }
            case TokenType::Literal:
                std::visit([&os, &token](const auto value) {
                    using ValueType = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<ValueType, semantic_analysis::StringId>) {
                        const std::string& identifier = semantic_analysis::StringTable::get().lookup(value);
                        os << token.type << "(" << identifier << ")";
                    } else if constexpr (!std::is_same_v<ValueType, std::monostate>) {
                        os << token.type << "(" << std::boolalpha << value << std::noboolalpha << ")";
                    }
                },
                    token.data);
                break;
            case TokenType::DataType:
                os << token.type << "(" << std::get<type_system::DataTypeKind>(token.data) << ")";
                break;
            default:
                log(log_type::INTERNAL_LEXING_WARNING, "Missing implementation of operator '<<' for token of type '", token.type, "'");
                break;
        }

        return os;
    }

}
