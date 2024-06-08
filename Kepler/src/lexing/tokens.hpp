#pragma once

namespace Kepler::Lexing {

	enum class TokenType {
		Unknown,
		Separator,
		BinaryOperator,
	};

	class IToken {
	private:
		std::string m_value;

	public:
		IToken(std::string value);
		std::string getValue() const;
		virtual TokenType getType() const;
	};

	class SeparatorToken : public IToken {
	public:
		using IToken::IToken;
		TokenType getType() const override;

		static bool matches(std::string word);
	};

	class BinaryOperatorToken : public IToken {
	public:
		using IToken::IToken;
		TokenType getType() const override;

		static bool matches(std::string word);
	};

	class UnknownToken : public IToken {
		using IToken::IToken;
	};

}