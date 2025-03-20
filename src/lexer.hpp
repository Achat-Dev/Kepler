#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace Kepler::Lexer {

    enum TokenType {
        Token_EndOfFile = -1,

        // Keywords
        Token_Function = -2,
        Token_Extern = -3,

        // Values
        Token_Identifier = -4,
        Token_Number = -5,
    };

    int read_token();

}
