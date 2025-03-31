#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "lexer.hpp"
#include "parser.hpp"

namespace Kepler::Lexer {

    static int read_identifier();
    static int read_number();
    static int read_comment();

    static std::ifstream file;
    static char last_char = ' ';
    static std::string identifier = "";
    static double number_value = 0.0;

    const bool initialise(const char *filename) {
        file.open(filename);
        if (file.is_open()) {
            Parser::read_next_token();
            return true;
        }
        return false;
    }

    const void cleanup() {
        file.close();
    }

    const std::string get_identifier() {
        return identifier;
    }

    const double get_number_value() {
        return number_value;
    }

    const int read_token() {
        if (file.peek() == EOF) {
            std::cout << "{ TokenType: EndOfFile }" << std::endl;
            return TokenType::Token_EndOfFile;
        }

        while (isspace(last_char)) {
            file.get(last_char);
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
        file.get(last_char);

        std::cout << "{ TokenType: Character: " << (char)c << " }" << std::endl;
        return c;
    }

    static int read_identifier() {
        identifier = last_char;
        file.get(last_char);
        while (isalnum(last_char)) {
            identifier += last_char;
            file.get(last_char);
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

        std::cout << "{ TokenType: Identifier: " << identifier << " }" << std::endl;
        return TokenType::Token_Identifier;
    }

    static int read_number() {
        std::string value_string;
            do {
                value_string += last_char;
                file.get(last_char);
            } while (isdigit(last_char) || last_char == '.');

            number_value = strtod(value_string.c_str(), 0);

            std::cout << "{ TokenType: Number: " << number_value << " }" << std::endl;
            return TokenType::Token_Number;
    }

    static int read_comment() {
        do {
            file.get(last_char);
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return read_token();
        }

        std::cout << "{ TokenType: EndOfFile }" << std::endl;
        return TokenType::Token_EndOfFile;
    }

}
