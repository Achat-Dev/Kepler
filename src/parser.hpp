#pragma once

namespace Kepler::Parser {

    const int get_current_token();
    const int read_next_token();
    void handle_function();
    void handle_extern();
    void handle_top_level_expression();

}
