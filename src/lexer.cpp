#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "compiler.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "type.hpp"

namespace Kepler::Lexer {

    static int read_identifier();
    static int read_number();
    static int read_comment();

    static char last_char = ' ';
    static std::string identifier = "";
    static double number_value = 0.0;
    static TypeToken type_token = TypeToken::None;

    std::string get_identifier() {
        return identifier;
    }

    double get_number_value() {
        return number_value;
    }

    TypeToken get_type() {
        return type_token;
    }

    int read_token() {
        if (Compiler::get_file()->peek() == EOF) {
            log("{ TokenType: EndOfFile }");
            return Token::Token_EndOfFile;
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

        log("{ TokenType: Character: ", (char)c, " }");
        return c;
    }

    static int read_identifier() {
        identifier = last_char;
        last_char = Compiler::get_file()->read_next_char();
        while (isalnum(last_char) || last_char == '_') {
            identifier += last_char;
            last_char = Compiler::get_file()->read_next_char();
        }

        if (identifier == "extern") {
            log("{ TokenType: extern }");
            return Token::Token_Extern;
        }
        else if (identifier == "return") {
            log("{ TokenType: return }");
            return Token::Token_Return;
        }
        else if (identifier == "end") {
            log("{ TokenType: end }");
            return Token::Token_End;
        }
        else if (identifier == "if") {
            log("{ TokenType: if }");
            return Token::Token_If;
        }
        else if (identifier == "else") {
            log("{ TokenType: else }");
            return Token::Token_Else;
        }
        else if (identifier == "elseif") {
            log("{ TokenType: elseif }");
            return Token::Token_Elseif;
        }
        else if (identifier == "for") {
            log("{ TokenType: for }");
            return Token::Token_For;
        }
        else if (identifier == "var") {
            log("{ TokenType: var }");
            type_token = TypeToken::Var;
            return Token::Token_DataType;
        }
        else if (identifier == "bool") {
            log("{ TokenType: bool }");
            type_token = TypeToken::Bool;
            return Token::Token_DataType;
        }
        else if (identifier == "char") {
            log("{ TokenType: char }");
            type_token = TypeToken::Char;
            return Token::Token_DataType;
        }
        else if (identifier == "string") {
            log("{ TokenType: string }");
            type_token = TypeToken::String;
            return Token::Token_DataType;
        }
        else if (identifier == "i8") {
            log("{ TokenType: i8 }");
            type_token = TypeToken::Int8;
            return Token::Token_DataType;
        }
        else if (identifier == "i16") {
            log("{ TokenType: i16 }");
            type_token = TypeToken::Int16;
            return Token::Token_DataType;
        }
        else if (identifier == "i32") {
            log("{ TokenType: i32 }");
            type_token = TypeToken::Int32;
            return Token::Token_DataType;
        }
        else if (identifier == "i64") {
            log("{ TokenType: i64 }");
            type_token = TypeToken::Int64;
            return Token::Token_DataType;
        }
        else if (identifier == "f32") {
            log("{ TokenType: f32 }");
            type_token = TypeToken::Float32;
            return Token::Token_DataType;
        }
        else if (identifier == "f64") {
            log("{ TokenType: f64 }");
            type_token = TypeToken::Float64;
            return Token::Token_DataType;
        }

        log("{ TokenType: identifier: '", identifier, "' }");
        return Token::Token_Identifier;
    }

    static int read_number() {
        std::string value_string;
        do {
            value_string += last_char;
            last_char = Compiler::get_file()->read_next_char();
        } while (isdigit(last_char) || last_char == '.');

        number_value = strtod(value_string.c_str(), 0);

        log("{ TokenType: number: ", number_value, " }");
        return Token::Token_Number;
    }

    static int read_comment() {
        last_char = Compiler::get_file()->read_next_char();

        // Two # after each other -> multiline comment
        if (last_char == '#') {
            while (!(last_char == '#' && Compiler::get_file()->peek() == '#')) {
                if (Compiler::get_file()->peek() == EOF) {
                    log(LogStyle::WARNING, "[ Lexing warning ]", LogStyle::DEFAULT, ": multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.");
                    log("{ TokenType: EndOfFile }");
                    return Token::Token_EndOfFile;
                }

                last_char = Compiler::get_file()->read_next_char();
            }

            Compiler::get_file()->read_next_char(); // eat second '#'
            last_char = Compiler::get_file()->read_next_char(); // save actual next character for reading next token
        }
        // Single line comment
        else {
            while (last_char != '\n' && last_char != '\r') {
                if (Compiler::get_file()->peek() == EOF) {
                    log("{ TokenType: EndOfFile }");
                    return Token::Token_EndOfFile;
                }

                last_char = Compiler::get_file()->read_next_char();
            }
        }

        return read_token();
    }

}
