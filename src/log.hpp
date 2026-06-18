// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "arguments.hpp"
#include <iostream>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler {

    namespace LogStyle {

        const std::string DEFAULT = "\033[0m";
        const std::string BOLD = "\033[1m";

        // Text colours
        const std::string BLACK = "\033[30m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string WHITE = "\033[37m";

        // Background colours
        const std::string BG_BLACK = "\033[40m";
        const std::string BG_RED = "\033[41m";
        const std::string BG_GREEN = "\033[42m";
        const std::string BG_YELLOW = "\033[43m";
        const std::string BG_BLUE = "\033[44m";
        const std::string BG_MAGENTA = "\033[45m";
        const std::string BG_CYAN = "\033[46m";
        const std::string BG_WHITE = "\033[47m";

        // Customs
        const std::string WARNING = BG_YELLOW + BLACK + BOLD;
        const std::string ERROR = BG_RED + BLACK + BOLD;
        const std::string UNSUPPORTED = BG_MAGENTA + BLACK + BOLD;
        const std::string SUCCESS = BG_GREEN + BLACK + BOLD;

    }

    template <typename T>
    void log(T t) {
        std::cout << t << LogStyle::DEFAULT << std::endl;
    }

    template <typename T, typename... Args>
    void log(T t, Args... args) {
        std::cout << t;
        log(args...);
    }

    template <typename... Args>
    void log_verbose(Args... args) {
        if (Arguments::should_log_verbose()) {
            log(args...);
        }
    }

}
