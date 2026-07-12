// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "log.hpp"
#include <cstddef>
#include <cstdlib>
#include <format>
#include <print>
#include <utility>

namespace kepler {

    template <typename... Args>
    [[noreturn]]
    void emergency_exit(std::format_string<Args...> format, Args&&... args) {
        std::println("{}{}[ Internal emergency, we have entered unreachable territory, everybody panic ]{}: {}", log::styling::bold, log::styling::bg_magenta, log::styling::reset, std::format(format, std::forward<Args>(args)...));

        constexpr char h[] = "\u2500";
        constexpr char v[] = "\u2502";
        constexpr char tl[] = "\u250C";
        constexpr char tr[] = "\u2510";
        constexpr char bl[] = "\u2514";
        constexpr char br[] = "\u2518";

        std::string header;
        for (size_t i = 0; i < 16; i++) {
            header += h;
        }
        std::string footer;
        for (size_t i = 0; i < 50; i++) {
            footer += h;
        }
        std::println("{}{}[ Emergency exit ]{}{}", tl, header, header, tr);
        std::println("{}         Roses are red, violets are blue,         {}", v, v);
        std::println("{}    I reached some code that I never should do.   {}", v, v);
        std::println("{}       Now here I am, with no helping hand,       {}", v, v);
        std::println("{}  And a crash and stack trace I don’t understand. {}", v, v);
        std::println("{}{}{}", bl, footer, br);

        std::abort();
    }

}
