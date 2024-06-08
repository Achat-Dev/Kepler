#pragma once

#include <string>
#include <vector>

#include "tokens.hpp"

namespace Kepler::Lexing {

	std::vector<IToken*> tokenize(std::string src);

}
