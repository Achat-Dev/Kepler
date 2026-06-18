// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/variable_definition_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "variables/local_variables.hpp"

#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <string>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableDefinitionExpression::codegen() {
        llvm::Function* f = Compiler::get().get_builder().GetInsertBlock()->getParent();
        if (!f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable defined outside of function");
            return ExpressionResult::create_invalid();
        }

        if (LocalVariables::get(name)) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable '", name, "' already exists");
            return ExpressionResult::create_invalid();
        }

        llvm::AllocaInst* alloca = create_entry_block_alloca(f, type, name);
        LocalVariables::set(name, { type, alloca });

        assert(value->get_operator() == Lexer::Token::Assignment && "[ Assertion ]: operator of variable assignment has to be '='");

        // Since this is a BinaryExpression, codegen handles the assignment and error handling
        std::unique_ptr<ExpressionResult> value_er = value->codegen();

        return std::move(value_er);
    }

    const std::string& VariableDefinitionExpression::get_name() const {
        return name;
    }

}
