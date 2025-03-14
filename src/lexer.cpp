#include <algorithm>
#include <iostream>
#include <sstream>

#include "lexer.hpp"

namespace Kepler::Lexer {

#define PRINT_TOKEN_TYPE(type) case type: return out << #type;

	std::ostream& operator<<(std::ostream& out, const TokenType value) {
		switch (value)
		{
			PRINT_TOKEN_TYPE(TokenType::Separator);
			PRINT_TOKEN_TYPE(TokenType::BinaryOperator);
		default: PRINT_TOKEN_TYPE(TokenType::Unknown);
		}
	}

	std::ostream& operator<<(std::ostream& out, const Token& value) {
		return out << "{ \"" << value.Type << "\" , \"" << value.Value << "\" }";
	}

	const std::string c_separators[] = {
		"(", ")"
	};

	const std::string c_binaryOperators[] = {
	   "+", "-", "*", "/", "="
	};

	template<typename T>
	bool matchToken(const T& tokens, const std::string& word) {
		const auto begin = std::begin(tokens);
		const auto end = std::end(tokens);
		const auto element = std::find(begin, end, word);
		return element != end;
	}

	std::vector<Token> tokenize(const std::string& src) {
		std::vector<Token> tokens;

		std::istringstream stream(src);

		do {
			std::string word;

			stream >> word;

			if (word.empty()) {
				continue;
			}

			if (matchToken(c_separators, word)) {
				tokens.push_back({ TokenType::Separator, word });
			}
			else if (matchToken(c_binaryOperators, word)) {
				tokens.push_back({ TokenType::BinaryOperator, word });
			}
			else {
				tokens.push_back({ TokenType::Unknown, word });
			}
		} while (stream);

		return tokens;
	}

}
