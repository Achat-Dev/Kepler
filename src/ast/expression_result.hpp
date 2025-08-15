// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "types/type_token.hpp"

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    enum ExpressionResultFlags {
        Valid = 0b0001,
        Returnable = 0b0010,
        Return = 0b0100,
        QualifiedReturn = 0b1000
    };

    class ExpressionResult {
    private:
        llvm::Value* value;
        const Type::TypeToken type;
        const unsigned int flags;

        ExpressionResult(llvm::Value* value, Type::TypeToken type, unsigned int flags)
            : value(value), type(type), flags(flags) {}

    public:
        bool is_valid() const;
        bool is_return_statement() const;
        bool is_returnable() const;
        bool is_assignable() const;
        bool forms_qualified_return() const;
        llvm::Value* get_value() const;
        void set_value(llvm::Value* new_value);
        Type::TypeToken get_type() const;
        unsigned int get_flags() const;

        static std::unique_ptr<ExpressionResult> create(llvm::Value* value, Type::TypeToken type, unsigned int flags);
        static std::unique_ptr<ExpressionResult> create_invalid();
    };

}
