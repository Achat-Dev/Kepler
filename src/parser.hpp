#pragma once

namespace Kepler::Parser {

    int get_current_token();
    int read_next_token();
    bool handle_data_type();
    bool handle_extern();

}
