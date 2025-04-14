#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "compiler.hpp"
#include "log.hpp"
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
        if (Compiler::get_file()->peek() == EOF) {
            log("{ TokenType: EndOfFile }");
            return TokenType::Token_EndOfFile;
        }

        while (isspace(last_char)) {
            last_char = Compiler::get_file()->read_next_char();
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
        last_char = Compiler::get_file()->read_next_char();

        log("{ TokenType: Character:", (char)c, "}");
        return c;
    }

    static int read_identifier() {
        identifier = last_char;
        last_char = Compiler::get_file()->read_next_char();
        while (isalnum(last_char) || last_char == '_') {
            identifier += last_char;
            last_char = Compiler::get_file()->read_next_char();
        }

        if (identifier == "function") {
            log("{ TokenType: function }");
            return TokenType::Token_Function;
        }
        else if (identifier == "extern") {
            log("{ TokenType: extern }");
            return TokenType::Token_Extern;
        }
        else if (identifier == "return") {
            log("{ TokenType: return }");
            return TokenType::Token_Return;
        }
        else if (identifier == "end") {
            log("{ TokenType: end }");
            return TokenType::Token_End;
        }
        else if (identifier == "if") {
            log("{ TokenType: if }");
            return TokenType::Token_If;
        }
        else if (identifier == "elseif") {
            log("{ TokenType: elseif }");
            return TokenType::Token_Elseif;
        }
        else if (identifier == "else") {
            log("{ TokenType: else }");
            return TokenType::Token_Else;
        }
        else if (identifier == "for") {
            log("{ TokenType: for }");
            return TokenType::Token_For;
        }

        log("{ TokenType: identifier", identifier, "}");
        return TokenType::Token_Identifier;
    }

    static int read_number() {
        std::string value_string;
            do {
                value_string += last_char;
                last_char = Compiler::get_file()->read_next_char();
            } while (isdigit(last_char) || last_char == '.');

            number_value = strtod(value_string.c_str(), 0);

            log("{ TokenType: number", number_value, "}");
            return TokenType::Token_Number;
    }

    static int read_comment() {
        do {
            last_char = Compiler::get_file()->read_next_char();
        } while (last_char != EOF && last_char != '\n' && last_char != '\r');

        if (last_char != EOF) {
            return read_token();
        }

        log("{ TokenType: EndOfFile }");
        return TokenType::Token_EndOfFile;
    }

}
