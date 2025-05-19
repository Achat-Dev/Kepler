#pragma once

#include "lexer.hpp"

namespace Kepler::Parser {

    Lexer::Token get_current_token();
    Lexer::Token read_next_token();
    bool handle_data_type();
    bool handle_extern();

}
