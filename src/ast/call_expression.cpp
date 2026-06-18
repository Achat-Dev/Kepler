// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/call_expression.hpp"

#include "ast/expression_result.hpp"
#include "ast/parameter_data.hpp"
#include "ast/prototype.hpp"
#include "compiler.hpp"
#include "function_registry/function_registry.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type_token.hpp"

#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <vector>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> CallExpression::codegen() {
        llvm::Function* callee_f = Compiler::get().get_module().getFunction(callee);
        if (!callee_f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown function '", callee, "' called");
            return ExpressionResult::create_invalid();
        }

        if (callee_f->arg_size() != args.size()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": incorrect number of arguments passed to function");
            return ExpressionResult::create_invalid();
        }

        std::shared_ptr<Prototype> prototype = FunctionRegistry::get_registered_prototype(callee);
        assert(prototype != nullptr && "[ Assertion ]: protoype called from CallExpression not registered as known prototype");

        const std::vector<ParameterData>& parameters = prototype->get_parameters();
        assert(args.size() == parameters.size() && "[ Assertion ]: argument count of CallExpression and respective prototype don't match");

        std::vector<llvm::Value*> args_v;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            Type::TargetTypeStack::push(parameters[i].type);
            std::unique_ptr<ExpressionResult> arg_er = args[i]->codegen();
            if (!arg_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in function argument");
                return ExpressionResult::create_invalid();
            }
            if (arg_er->get_type() != parameters[i].type) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: function argument expected value of type '", parameters[i].type, "', but was given value of type '", arg_er->get_type(), '\'');
                return ExpressionResult::create_invalid();
            }

            args_v.push_back(arg_er->get_value());
            if (!args_v.back()) {
                return ExpressionResult::create_invalid();
            }
            Type::TargetTypeStack::pop();
        }

        if (prototype->get_type() == Type::TypeToken::Void) {
            return ExpressionResult::create(Compiler::get().get_builder().CreateCall(callee_f, args_v), prototype->get_type(), ExpressionResultFlags::Valid);
        }
        return ExpressionResult::create(Compiler::get().get_builder().CreateCall(callee_f, args_v, "calltmp"), prototype->get_type(), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
