#include <cstdio>
#include <iostream>
#include <ostream>

#include "lexer.hpp"
#include "parser.hpp"

using namespace Kepler;

bool compile_file(const char* filename);

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout << "Error: no filename given" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    if (!compile_file(filename)) {
        std::cout << "Failed to open file '" << filename << "'" << std::endl;
        return 1;
    }

	return 0;
}

bool compile_file(const char* filename) {
    if (!Lexer::initialise(filename)) {
        return false;
    }

    while (true) {
        switch (Parser::get_current_token()) {
            case Lexer::Token_EndOfFile:
                Lexer::cleanup();
                return true;
            case Lexer::Token_Function:
                Parser::handle_function();
                break;
            case Lexer::Token_Extern:
                Parser::handle_extern();
                break;
            default:
                Parser::handle_top_level_expression();
                break;
        }
    }
}
