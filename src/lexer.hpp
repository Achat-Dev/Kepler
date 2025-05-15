#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include "types/type.hpp"

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
        Token_Float_Value = -10,
        Token_Int_Value = -11,
        Token_DataType = -12,

        // Parsing tokens
        // These are necessary for certain parsing operations
        Token_Parsing_Elseif = std::numeric_limits<int32_t>::max(),
    };

    int read_token();
    std::string get_identifier();
    int64_t get_int_value();
    double get_float_value();
    Type::TypeToken get_type();

}
