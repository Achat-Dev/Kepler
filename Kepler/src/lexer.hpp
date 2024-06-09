#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace Kepler::Lexer {

	enum class TokenType {
		Unknown,
		Identifier,
		Separator,
		BinaryOperator,
		Keyword,
		DataType,
	};

	struct Token {
		TokenType Type;
		std::string Value;
	};

	std::ostream& operator<<(std::ostream& out, const TokenType value);
	std::ostream& operator<<(std::ostream& out, const Token& value);

	std::vector<Token> tokenize(const std::string& src);

}
