// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/negation_expression.hpp"

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> NegationExpression::codegen() {
        const Type::TypeToken target_type = Type::TargetTypeStack::top();
        const bool should_target_default_type = !Type::is_floating_point_type(target_type) && !Type::is_integer_type(target_type);
        if (should_target_default_type) {
            Type::TargetTypeStack::push(Type::TypeToken::None);
        }

        std::unique_ptr<ExpressionResult> value_er = value->codegen();
        if (!value_er->is_valid()) {
            return ExpressionResult::create_invalid();
        }

        if (should_target_default_type) {
            Type::TargetTypeStack::pop();
        }

        const Type::TypeToken type = value_er->get_type();
        if (Type::is_integer_type(type)) {
            return ExpressionResult::create(Compiler::get_builder().CreateNeg(value_er->get_value(), "neg"), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            return ExpressionResult::create(Compiler::get_builder().CreateFNeg(value_er->get_value(), "neg"), type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": mathematical negation of type '", type, "' is not supported");
        return ExpressionResult::create_invalid();
    }

}
