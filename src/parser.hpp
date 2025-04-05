#pragma once

namespace Kepler::Parser {

    const int get_current_token();
    const int read_next_token();
    const bool handle_function();
    const bool handle_extern();
    const bool handle_top_level_expression();

}
