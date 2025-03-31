#pragma once

#include <string>

namespace Kepler::Lexer {

    enum TokenType {
        Token_EndOfFile = -1,

        // Keywords
        Token_Function = -2,
        Token_Extern = -3,

        // Primary
        Token_Identifier = -4,
        Token_Number = -5,
        Token_If = -6,
        Token_Elseif = -7,
        Token_Else = -8
    };

    const bool initialise(const char* filename);
    const void cleanup();

    const int read_token();
    const std::string get_identifier();
    const double get_number_value();

}
