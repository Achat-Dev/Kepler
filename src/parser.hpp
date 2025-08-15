// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "lexer.hpp"

namespace Kepler::Parser {

    Lexer::Token get_current_token();
    Lexer::Token read_next_token();
    bool handle_top_level_data_type();
    bool handle_top_level_extern();

}
