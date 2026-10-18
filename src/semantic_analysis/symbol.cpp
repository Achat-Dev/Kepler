// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/symbol.hpp"
#include "utils/assert.h"

namespace kepler {

    bool is_overloadable_symbol(SymbolKind symbol_kind) {
        switch (symbol_kind) {
            case SymbolKind::All:
                KPL_ASSERT_UNREACHABLE("Cannot determine overloadable for symbol kind '{}'", symbol_kind);
            case SymbolKind::Variable:
                return false;
            case SymbolKind::Prototype:
                return true;
            case SymbolKind::Type:
                return false;
        }
        KPL_ASSERT_UNREACHABLE("Missing is overloadable symbol implementation for symbol kind '{}'", static_cast<int>(symbol_kind));
    }

}
