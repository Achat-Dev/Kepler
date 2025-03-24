#include <cstdio>

#include "lexer.hpp"
#include "parser.hpp"

using namespace Kepler;

int main() {
    std::fprintf(stderr, "ready > ");
    Parser::read_next_token();

    while (true) {
        std::fprintf(stderr, "ready > ");
        switch (Parser::get_current_token()) {
            case Lexer::Token_EndOfFile:
                return 0;
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
	return 0;
}
