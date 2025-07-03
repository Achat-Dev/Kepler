#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>

#include "compiler.hpp"
#include "lexer.hpp"
#include "types/type.hpp"
#include "log.hpp"

namespace Kepler::Lexer {

    static Token read_identifier();
    static Token read_number_literal();
    static Token read_string_literal();
    static Token read_comment();

    static char last_char = ' ';
    static std::string identifier = "";
    static int64_t integer_literal = 0;
    static double floating_point_literal = 0.0;
    static std::string string_literal = "";
    static Type::TypeToken type_token = Type::TypeToken::None;

    std::ostream& operator<<(std::ostream& os, Token token) {
        switch (token) {
            case Token::EndOfFile: os << "EndOfFile"; break;
            case Token::Unknown: os << "unknown: '" << last_char << '\''; break;
            case Token::BracketOpen: os << '('; break;
            case Token::BracketClose: os << ')'; break;
            case Token::Comma: os << ','; break;
            case Token::Colon: os << ':'; break;
            case Token::Assignment: os << '='; break;
            case Token::Plus: os << '+'; break;
            case Token::Minus: os << '-'; break;
            case Token::Multiplication: os << '*'; break;
            case Token::Division: os << '/'; break;
            case Token::LessThan: os << '<'; break;
            case Token::GreaterThan: os << '>'; break;
            case Token::Equals: os << "=="; break;
            case Token::NotEquals: os << "!="; break;
            case Token::LessEquals: os << "<="; break;
            case Token::GreaterEquals: os << ">="; break;
            case Token::Extern: os << "extern"; break;
            case Token::Return: os << "return"; break;
            case Token::End: os << "end"; break;
            case Token::If: os << "if"; break;
            case Token::Else: os << "else"; break;
            case Token::Elseif: os << "elseif"; break;
            case Token::For: os << "for"; break;
            case Token::Identifier: os << "identifier"; break;
            case Token::FloatingPointLiteral: os << "floating point literal"; break;
            case Token::IntegerLiteral: os << "integer literal"; break;
            case Token::StringLiteral: os << "string literal"; break;
            case Token::DataType: os << "data type"; break;
            default: log(LogStyle::WARNING, "[ Lexing warning ]", LogStyle::DEFAULT, ": missing implementation of operator '<<' for token '", (int)token, '\''); break;
        }

        return os;
    }

    std::string get_identifier() {
        return identifier;
    }

    int64_t get_integer_literal() {
        return integer_literal;
    }

    double get_floating_point_literal() {
        return floating_point_literal;
    }

    std::string get_string_literal() {
        return string_literal;
    }

    Type::TypeToken get_type() {
        return type_token;
    }

    Token read_token() {
        if (Compiler::get_file()->peek() == EOF) {
            log("{ TokenType: EndOfFile }");
            return Token::EndOfFile;
        }

        while (isspace(last_char)) {
            last_char = Compiler::get_file()->read_next_char();
        }

        if (isalpha(last_char)) {
            return read_identifier();
        }
        if (isdigit(last_char)) {
            return read_number_literal();
        }

        switch (last_char) {
            case '#': return read_comment();
            case ',':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Comma;
            case ':':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Colon;
            case '(':
                last_char = Compiler::get_file()->read_next_char();
                return Token::BracketOpen;
            case ')':
                last_char = Compiler::get_file()->read_next_char();
                return Token::BracketClose;
            case '=':
                last_char = Compiler::get_file()->read_next_char();
                if (last_char == '=') {
                    last_char = Compiler::get_file()->read_next_char();
                    return Token::Equals;
                }
                else {
                    return Token::Assignment;
                }
            case '+':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Plus;
            case '-':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Minus;
            case '*':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Multiplication;
            case '/':
                last_char = Compiler::get_file()->read_next_char();
                return Token::Division;
            case '<':
                last_char = Compiler::get_file()->read_next_char();
                if (last_char == '=') {
                    last_char = Compiler::get_file()->read_next_char();
                    return Token::LessEquals;
                }
                else {
                    return Token::LessThan;
                }
            case '>':
                last_char = Compiler::get_file()->read_next_char();
                if (last_char == '=') {
                    last_char = Compiler::get_file()->read_next_char();
                    return Token::GreaterEquals;
                }
                else {
                    return Token::GreaterThan;
                }
            case '!':
                last_char = Compiler::get_file()->read_next_char();
                if (last_char == '=') {
                    last_char = Compiler::get_file()->read_next_char();
                    return Token::NotEquals;
                }
                else {
                    log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": logical negation with '!' is not supported yet");
                    last_char = '!';
                    break;
                }
            case '"':
                return read_string_literal();
        }

        log(LogStyle::ERROR, "[ Lexing error ]", LogStyle::DEFAULT, ": unknown character '", last_char, "' while lexing");
        return Token::Unknown;
    }

    static Token read_identifier() {
        identifier = last_char;
        last_char = Compiler::get_file()->read_next_char();
        while (isalnum(last_char) || last_char == '_') {
            identifier += last_char;
            last_char = Compiler::get_file()->read_next_char();
        }

        if (identifier == "extern") {
            log("{ TokenType: extern }");
            return Token::Extern;
        }
        else if (identifier == "return") {
            log("{ TokenType: return }");
            return Token::Return;
        }
        else if (identifier == "end") {
            log("{ TokenType: end }");
            return Token::End;
        }
        else if (identifier == "if") {
            log("{ TokenType: if }");
            return Token::If;
        }
        else if (identifier == "else") {
            log("{ TokenType: else }");
            return Token::Else;
        }
        else if (identifier == "elseif") {
            log("{ TokenType: elseif }");
            return Token::Elseif;
        }
        else if (identifier == "for") {
            log("{ TokenType: for }");
            return Token::For;
        }
        else if (identifier == "true") {
            log("{ TokenType: true }");
            return Token::True;
        }
        else if (identifier == "false") {
            log("{ TokenType: false }");
            return Token::False;
        }
        else if (identifier == "void") {
            log("{ TokenType: void }");
            type_token = Type::TypeToken::Void;
            return Token::DataType;
        }
        else if (identifier == "var") {
            log("{ TokenType: var }");
            type_token = Type::TypeToken::Var;
            return Token::DataType;
        }
        else if (identifier == "bool") {
            log("{ TokenType: bool }");
            type_token = Type::TypeToken::Bool;
            return Token::DataType;
        }
        else if (identifier == "char") {
            log("{ TokenType: char }");
            type_token = Type::TypeToken::Char;
            return Token::DataType;
        }
        else if (identifier == "string") {
            log("{ TokenType: string }");
            type_token = Type::TypeToken::String;
            return Token::DataType;
        }
        else if (identifier == "i8") {
            log("{ TokenType: i8 }");
            type_token = Type::TypeToken::Int8;
            return Token::DataType;
        }
        else if (identifier == "i16") {
            log("{ TokenType: i16 }");
            type_token = Type::TypeToken::Int16;
            return Token::DataType;
        }
        else if (identifier == "i32") {
            log("{ TokenType: i32 }");
            type_token = Type::TypeToken::Int32;
            return Token::DataType;
        }
        else if (identifier == "i64") {
            log("{ TokenType: i64 }");
            type_token = Type::TypeToken::Int64;
            return Token::DataType;
        }
        else if (identifier == "f32") {
            log("{ TokenType: f32 }");
            type_token = Type::TypeToken::Float32;
            return Token::DataType;
        }
        else if (identifier == "f64") {
            log("{ TokenType: f64 }");
            type_token = Type::TypeToken::Float64;
            return Token::DataType;
        }

        log("{ TokenType: identifier: '", identifier, "' }");
        return Token::Identifier;
    }

    static Token read_number_literal() {
        bool is_float = false;
        std::string literal_string;
        do {
            literal_string += last_char;
            last_char = Compiler::get_file()->read_next_char();
            if (last_char == '.') {
                is_float = true;
            }
        } while (isdigit(last_char) || last_char == '.');

        if (is_float) {
            floating_point_literal = std::stod(literal_string);
            log("{ TokenType: floating point literal: ", floating_point_literal, " }");
            return Token::FloatingPointLiteral;
        }
        else {
            integer_literal = std::stoll(literal_string);
            log("{ TokenType: integer literal: ", integer_literal, " }");
            return Token::IntegerLiteral;
        }
    }

    static Token read_string_literal() {
        string_literal = "";
        last_char = Compiler::get_file()->read_next_char();

        while (last_char != '"') {
            if (last_char == '\\') {
                last_char = Compiler::get_file()->read_next_char(); // read the character to escape

                switch (last_char) {
                    case 'n': string_literal += '\n'; break;
                    case 't': string_literal += '\t'; break;
                    case '\\': string_literal += '\\'; break;
                    case '"': string_literal += '"'; break;
                    default:
                        log(LogStyle::ERROR, "[ Lexing error ]", LogStyle::DEFAULT, ": unknown escape character '\\", last_char, "' in string");
                        return Token::Unknown;
                }

                last_char = Compiler::get_file()->read_next_char(); // read the next character for the next loop interation
            }
            else {
                string_literal += last_char;
                last_char = Compiler::get_file()->read_next_char();
            }
        }

        last_char = Compiler::get_file()->read_next_char(); // eat closing '"'
        return Token::StringLiteral;
    }

    static Token read_comment() {
        last_char = Compiler::get_file()->read_next_char();

        // Two # after each other -> multiline comment
        if (last_char == '#') {
            while (!(last_char == '#' && Compiler::get_file()->peek() == '#')) {
                if (Compiler::get_file()->peek() == EOF) {
                    log(LogStyle::WARNING, "[ Lexing warning ]", LogStyle::DEFAULT, ": multiline comment is not closed. This file may stil compile without issues, but consider closing the comment.");
                    log("{ TokenType: EndOfFile }");
                    return Token::EndOfFile;
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
                    return Token::EndOfFile;
                }

                last_char = Compiler::get_file()->read_next_char();
            }
        }

        return read_token();
    }

}
