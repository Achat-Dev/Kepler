// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"

// TODO (improvement): Enfore that structs and methods need to have the same export declaration
// TODO (improvement): Enfore that there can only be one implementation of an operator
int main(int argc, char** argv) {
    kepler::Compiler compiler;
    return compiler.run(argc, argv);
}
