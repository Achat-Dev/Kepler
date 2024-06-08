#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "tokens.hpp"

namespace Kepler::Lexing {

	std::vector<IToken*> tokenize(std::string src) {
		std::vector<IToken*> tokens;

		std::istringstream stream(src);

		do {
			std::string word;

			stream >> word;

			if (word.empty()) {
				continue;
			}

			if (SeparatorToken::matches(word)) {
				tokens.push_back(new SeparatorToken(word));
			}
			else if (BinaryOperatorToken::matches(word)) {
				tokens.push_back(new BinaryOperatorToken(word));
			}
			else {
				tokens.push_back(new UnknownToken(word));
			}

		} while (stream);

		return tokens;
	}

}