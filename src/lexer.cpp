#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "lexer.hpp"

namespace Kepler::Lexer {

    static int read_identifier(int* last_char);
    static int read_number(int* last_char);
    static int read_comment(int* last_char);

    static std::string identifier;
    static double value;

    int read_token() {
        static int last_char = ' ';

        while (isspace(last_char)) {
            last_char = getchar();
        }

        if (isalpha(last_char)) {
            return read_identifier(&last_char);
        }
        if (isdigit(last_char)) {
            return read_number(&last_char);
        }
        if (last_char == '#') {
            return read_comment(&last_char);
        }
        if (last_char == EOF) {
            return TokenType::Token_EndOfFile;
        }

        int c = last_char;
        last_char = getchar();
        return c;
    }

    static int read_identifier(int* last_char) {
        identifier = *last_char;
        while (isalnum((*last_char = getchar()))) {
            identifier += *last_char;
        }

        if (identifier == "function") {
            return TokenType::Token_Function;
        }
        else if (identifier == "extern") {
            return TokenType::Token_Extern;
        }
        return TokenType::Token_Identifier;
    }

    static int read_number(int* last_char) {

        std::string value_string;
            do {
                value_string += *last_char;
                *last_char = getchar();
            } while (isdigit(*last_char) || *last_char == '.');

            value = strtod(value_string.c_str(), 0);
            return TokenType::Token_Number;
    }

    static int read_comment(int* last_char) {
        do {
            *last_char = getchar();
        } while (*last_char != EOF && *last_char != '\n' && *last_char != '\r');

        if (*last_char != EOF) {
            return read_token();
        }
        return TokenType::Token_EndOfFile;
    }

}
