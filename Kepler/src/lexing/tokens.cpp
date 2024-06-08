#include <string>
#include "tokens.hpp"

namespace Kepler::Lexing {

	/* --------------- IToken --------------- */

	IToken::IToken(std::string value) : m_value(value) {

	}

	TokenType IToken::getType() const {
		return TokenType::Unknown;
	}

	std::string IToken::getValue() const {
		return m_value;
	}

	/* --------------- Separator token --------------- */

	TokenType SeparatorToken::getType() const {
		return TokenType::Separator;
	}

	bool SeparatorToken::matches(std::string word) {
		return word == "(" || word == ")";
	}

	/* --------------- Binary Operator token --------------- */

	TokenType BinaryOperatorToken::getType() const {
		return TokenType::BinaryOperator;
	}

	bool BinaryOperatorToken::matches(std::string word) {
		return word == "+" || word == "-" || word == "*" || word == "/";
	}

}