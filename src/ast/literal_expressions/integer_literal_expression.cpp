// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/literal_expressions/integer_literal_expression.hpp"

#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <cstdint>
#include <limits>
#include <llvm/IR/Constants.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> IntegerLiteralExpression::codegen() {
        Type::TypeToken type = Type::TargetTypeStack::top();

        if (Type::is_integer_type(type)) {
            return ExpressionResult::create(llvm::ConstantInt::getSigned(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (Type::is_floating_point_type(type)) {
            return ExpressionResult::create(llvm::ConstantFP::get(Type::get_by_token(type), value), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        if (type == Type::TypeToken::None || type == Type::TypeToken::TMap) {
            if (value > std::numeric_limits<int32_t>::max()) {
                return ExpressionResult::create(llvm::ConstantInt::getSigned(Type::get_by_token(Type::TypeToken::Int64), value), Type::TypeToken::Int64, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
            }
            return ExpressionResult::create(llvm::ConstantInt::getSigned(Type::get_by_token(Type::TypeToken::Int32), value), Type::TypeToken::Int32, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from an integer literal");
        return ExpressionResult::create_invalid();
    }

    int64_t IntegerLiteralExpression::get_value() const {
        return value;
    }

}
