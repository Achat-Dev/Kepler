// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/expression_result.hpp"

#include "types/type_token.hpp"

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    bool ExpressionResult::is_valid() const {
        return flags & ExpressionResultFlags::Valid;
    }

    bool ExpressionResult::is_return_statement() const {
        return flags & ExpressionResultFlags::Return;
    }

    bool ExpressionResult::is_returnable() const {
        return flags & ExpressionResultFlags::Returnable;
    }

    bool ExpressionResult::is_assignable() const {
        return flags & ExpressionResultFlags::Returnable;
    }

    bool ExpressionResult::forms_qualified_return() const {
        return flags & ExpressionResultFlags::QualifiedReturn;
    }

    llvm::Value* ExpressionResult::get_value() const {
        return value;
    }

    void ExpressionResult::set_value(llvm::Value* new_value) {
        value = new_value;
    }

    unsigned int ExpressionResult::get_flags() const {
        return flags;
    }

    Type::TypeToken ExpressionResult::get_type() const {
        return type;
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create(llvm::Value* value, Type::TypeToken type, unsigned int flags) {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(value, type, flags));
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_invalid() {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(nullptr, Type::TypeToken::None, 0));
    }

}
