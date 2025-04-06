#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "compiler.hpp"
#include "lexer.hpp"

namespace Kepler::Lexer {

    static int read_identifier();
    static int read_number();
    static int read_comment();

    static char last_char = ' ';
    static std::string identifier = "";
    static double number_value = 0.0;

    const std::string get_identifier() {
        return identifier;
    }

    const double get_number_value() {
        return number_value;
    }

    const int read_token() {
        if (Compiler::Internal::get_file()->peek() == EOF) {
            std::cout << "{ TokenType: EndOfFile }" << std::endl;
            return TokenType::Token_EndOfFile;
        }

        while (isspace(last_char)) {
            last_char = Compiler::Internal::get_file()->read_next_char();
        }

        if (isalpha(last_char)) {
            return read_identifier();
        }
        if (isdigit(last_char)) {
            return read_number();
        }
        if (last_char == '#') {
            return read_comment();
        }

        int c = last_char;
        last_char = Compiler::Internal::get_file()->read_next_char();

        std::cout << "{ TokenType: Character: " << (char)c << " }" << std::endl;
        return c;
    }

    static int read_identifier() {
        identifier = last_char;
        last_char = Compiler::Internal::get_file()->read_next_char();
        while (isalnum(last_char)) {
            identifier += last_char;
            last_char = Compiler::Internal::get_file()->read_next_char();
        }

        if (identifier == "function") {
            std::cout << "{ TokenType: Function }" << std::endl;
            return TokenType::Token_Function;
        }
        else if (identifier == "extern") {
            std::cout << "{ TokenType: Extern }" << std::endl;
            return TokenType::Token_Extern;
        }
        else if (identifier == "if") {
            std::cout << "{ TokenType: if }" << std::endl;
            return TokenType::Token_If;
        }
        else if (identifier == "elseif") {
            std::cout << "{ TokenType: elseif }" << std::endl;
            return TokenType::Token_Elseif;
        }
        else if (identifier == "else") {
            std::cout << "{ TokenType: else }" << std::endl;
            return TokenType::Token_Else;
        }
        else if (identifier == "for") {
            std::cout << "{ TokenType: for }" << std::endl;
            return TokenType::Token_For;
        }

        std::cout << "{ TokenType: Identifier: " << identifier << " }" << std::endl;
        return TokenType::Token_Identifier;
    }

    static int read_number() {
        std::string value_string;
            do {
                value_string += last_char;
                last_char = Compiler::Internal::get_file()->read_next_char();
            } while (isdigit(last_char) || last_char == '.');

            number_value = strtod(value_string.c_str(), 0);

            std::cout << "{ TokenType: Number: " << number_value << " }" << std::endl;
            return TokenType::Token_Number;
    }

    static int read_comment() {
        do {
            last_char = Compiler::Internal::get_file()->read_next_char();
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return read_token();
        }

        std::cout << "{ TokenType: EndOfFile }" << std::endl;
        return TokenType::Token_EndOfFile;
    }

}
