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

#include "../compiler.hpp"
#include "../log.hpp"
#include "../optimiser.hpp"
#include "../utils.hpp"
#include "expression.hpp"
#include "expression_result.hpp"
#include "parameter_data.hpp"
#include "function.hpp"

namespace Kepler::AST {

    /*
    * This method currently has a bug:
    * If the FunctionAST::codegen() method finds an existing IR Function, it does not validate its signature against the definition’s own prototype. This means that an earlier ‘extern’ declaration will take precedence over the function definition’s signature, which can cause codegen to fail, for instance if the function arguments are named differently
    */
    llvm::Function* Function::codegen() {
        llvm::Function* f = Compiler::get_module().getFunction(prototype->get_name());
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

        llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(Compiler::get_context(), "entry", f);
        Compiler::get_builder().SetInsertPoint(entry_block);

        // record function arguments
        Compiler::get_local_variables().clear();
        for (llvm::Argument& arg : f->args()) {
            llvm::AllocaInst* alloca = create_entry_block_alloca(f, arg.getType(), arg.getName());
            Compiler::get_builder().CreateStore(&arg, alloca);
            const ParameterData& parameter_data = prototype->get_arg(arg.getName().str());
            Compiler::get_local_variables()[std::string(arg.getName())] = { parameter_data.type, alloca };
        }

        Compiler::set_function_return_type(prototype->get_type());

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

        Compiler::set_function_return_type(Type::TypeToken::None);

        llvm::EliminateUnreachableBlocks(*f);

        if (llvm::verifyFunction(*f, &llvm::errs())) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to verify function. This probably means that the function is missing a 'return' expression");
            f->print(llvm::errs());
            f->eraseFromParent();
            return nullptr;
        }

        Optimiser::optimise_function(*f);
        return f;
    }

}
