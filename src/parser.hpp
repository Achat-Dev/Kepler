#pragma once

#include "lexer.hpp"

namespace Kepler::Parser {

    Lexer::Token get_current_token();
    Lexer::Token read_next_token();
    bool handle_top_level_data_type();
    bool handle_top_level_extern();

}
