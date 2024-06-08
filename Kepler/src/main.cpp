#include <iostream>
#include <string>
#include <vector>
#include "lexing/lexer.hpp"

using namespace Kepler;

int main() {
	std::string src = "+ - ( $ * )";

	std::vector<Lexing::IToken*> tokens = Lexing::tokenize(src);

	for (size_t i = 0; i < tokens.size(); i++)
	{
		std::cout << "[" << (int)tokens[i]->getType() << ", " << tokens[i]->getValue() << "]" << std::endl;
		delete tokens[i];
	}

	tokens.clear();

	return 0;
}
