#include <iostream>
#include <string>
#include <vector>

#include "lexer.hpp"

using namespace Kepler;

int main() {
	std::string src = "+ - ( $ * )";

	std::vector<Lexer::Token> tokens = Lexer::tokenize(src);

	for (auto token : tokens) {
		std::cout << token << std::endl;
	}

	return 0;
}
