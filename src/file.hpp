// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <fstream>
#include <memory>
#include <string>

namespace Kepler {

    class File {
    private:
        std::ifstream stream;
        int current_line_number;

        File(): current_line_number(1) {}

    public:
        char read_next_char();
        int peek();
        void close();
        int get_current_line_number() const;

        static std::unique_ptr<File> create(const std::string& filename);
    };

}
