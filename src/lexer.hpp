#pragma once

#include <string>

namespace Kepler::Lexer {

    enum TokenType {
        Token_EndOfFile = -1,

        // Keywords
        Token_Function = -2,
        Token_Extern = -3,
        Token_Return = -4,
        Token_End = -5,

        // Primary
        Token_Identifier = -6,
        Token_Number = -7,
        Token_If = -8,
        Token_Else = -9,
        Token_For  =-10
    };

    const int read_token();
    const std::string get_identifier();
    const double get_number_value();

}
