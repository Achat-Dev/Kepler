// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/variable_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"
#include "variables/local_variables.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Instructions.h>
#include <optional>
#include <string>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableExpression::codegen() {
        std::optional<LocalVariables::VariableData> variable_data = LocalVariables::get(name);
        if (!variable_data) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown variable name '", name, '\'');
            return ExpressionResult::create_invalid();
        }

        Type::TypeToken target_type = Type::TargetTypeStack::top();
        if (target_type != Type::TypeToken::None && target_type != Type::TypeToken::TMap && target_type != variable_data->type) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: accessed variable is of type '", variable_data->type, "', but expected type is '", target_type, '\'');
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(Compiler::get().get_builder().CreateLoad(Type::get_by_token(variable_data->type), variable_data->variable, name.c_str()), variable_data->type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

    const std::string& VariableExpression::get_name() const {
        return name;
    }

}
