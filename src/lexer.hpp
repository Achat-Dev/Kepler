#pragma once

#include <climits>
#include <string>

#include "type.hpp"

namespace Kepler::Lexer {

    enum Token {
        Token_EndOfFile = -1,

        // Keywords
        Token_Extern = -2,
        Token_Return = -3,
        Token_End = -4,
        Token_If = -5,
        Token_Else = -6,
        Token_Elseif = -7,
        Token_For = -8,

        // Primary
        Token_Identifier = -9,
        Token_Number = -10,
        Token_DataType = -11,

        // Parsing tokens
        // These are necessary for certain parsing operations
        Token_Parsing_Elseif = INT_MIN,
    };

    int read_token();
    std::string get_identifier();
    double get_number_value();
    TypeToken get_type();

}
