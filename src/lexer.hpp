#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "types/type.hpp"

namespace Kepler::Lexer {

    enum class Token {
        // Meta
        EndOfFile,
        BracketOpen,
        BracketClose,
        Comma,
        Colon,

        // Operators
        Assignment,
        Plus,
        Minus,
        Multiplication,
        Division,
        LessThan,
        GreaterThan,

        // Keywords
        Extern,
        Return,
        End,
        If,
        Else,
        Elseif,
        For,

        // Primary
        Identifier,
        FloatValue,
        IntValue,
        DataType,

        // Parsing tokens
        // These are necessary for certain parsing operations
        Parsing_Elseif,
    };

    std::ostream& operator<<(std::ostream& os, Token token);

    Token read_token();
    std::string get_identifier();
    int64_t get_int_value();
    double get_float_value();
    Type::TypeToken get_type();

}
