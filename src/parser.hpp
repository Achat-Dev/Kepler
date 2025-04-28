#pragma once

namespace Kepler::Parser {

    int get_current_token();
    int read_next_token();
    bool handle_function();
    bool handle_extern();
    bool handle_top_level_expression();

}
