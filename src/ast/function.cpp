// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "ast/function.hpp"

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "ast/parameter_data.hpp"
#include "compiler.hpp"
#include "function_registry/function_registry.hpp"
#include "log.hpp"
#include "optimiser.hpp"
#include "types/type_token.hpp"
#include "utils.hpp"
#include "variables/local_variables.hpp"

#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <memory>
#include <string>

namespace Kepler::AST {

    llvm::Function* Function::codegen() {
        llvm::Function* f = Compiler::get().get_module().getFunction(prototype->get_name());
        if (!f) {
            f = prototype->codegen();
        }

        if (!f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to compile function prototype");
            return nullptr;
        }

        if (!f->empty()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": function cannot be redefined");
            return nullptr;
        }

        llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(Compiler::get().get_context(), "entry", f);
        Compiler::get().get_builder().SetInsertPoint(entry_block);

        // Record function arguments
        LocalVariables::clear();
        for (llvm::Argument& arg : f->args()) {
            const ParameterData& parameter_data = prototype->get_parameter(arg.getName().str());
            llvm::AllocaInst* alloca = create_entry_block_alloca(f, parameter_data.type, arg.getName());
            Compiler::get().get_builder().CreateStore(&arg, alloca);
            LocalVariables::set(std::string(arg.getName()), { parameter_data.type, alloca });
        }

        // Codegen function body
        for (int i = 0; i < body.size(); i++) {
            std::unique_ptr<ExpressionResult> body_er = body[i]->codegen();
            if (!body_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in function");
                f->eraseFromParent();
                return nullptr;
            }
            if (body_er->is_return_statement() || body_er->forms_qualified_return()) {
                if (body.size() - (i + 1) > 0) {
                    log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": unreachable code in function detected");
                }
                break;
            }
        }

        FunctionRegistry::set_current_prototype(nullptr);

        llvm::EliminateUnreachableBlocks(*f);

        if (Compiler::get().get_builder().GetInsertBlock()->getTerminator() == nullptr) {
            if (prototype->get_type() != Type::TypeToken::Void) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": function is missing a 'return' expression");
                f->print(llvm::errs());
                f->eraseFromParent();
                return nullptr;
            }

            Compiler::get().get_builder().CreateRetVoid();
        }

        if (llvm::verifyFunction(*f, &llvm::errs())) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to verify function");
            f->print(llvm::errs());
            f->eraseFromParent();
            return nullptr;
        }

        Optimiser::optimise_function(*f);
        return f;
    }

}
