// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

namespace kepler {

    void create_builtin_types(SymbolTable& symbol_table, TypeTable& type_table);
    SymbolId get_builtin_type_symbol_id(TypeKind type_kind);
    TypeId get_builtin_type_id(TypeKind type_kind);
    llvm::Type* get_builtin_llvm_type(TypeKind type_kind, llvm::LLVMContext& context);

}
