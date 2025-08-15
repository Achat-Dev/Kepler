// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <string>
#include <vector>
namespace Kepler::Arguments {

    enum class ArgumentParseResult {
        HELP,
        ERROR,
        SUCCESS,
    };

    ArgumentParseResult parse(int argc, char* argv[]);

    const std::string& get_input_file();
    const std::string& get_output_file();
    const std::vector<std::string>& get_additional_files();
    const bool should_log_verbose();

}
