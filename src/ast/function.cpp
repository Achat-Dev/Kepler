#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../optimiser.hpp"
#include "function.hpp"

namespace Kepler::AST {

    /*
    * This method currently has a bug:
    * If the FunctionAST::codegen() method finds an existing IR Function, it does not validate its signature against the definition’s own prototype. This means that an earlier ‘extern’ declaration will take precedence over the function definition’s signature, which can cause codegen to fail, for instance if the function arguments are named differently
    */
    llvm::Function* Function::codegen() {
        llvm::Function* f = Compiler::Internal::get_module().getFunction(prototype->get_name());
        if (!f) {
            f = prototype->codegen();
        }

        if (!f) {
            return nullptr;
        }

        if (!f->empty()) {
            return static_cast<llvm::Function*>(log_errorv("function cannot be redefined"));
        }

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(Compiler::Internal::get_context(), "entry", f);
        Compiler::Internal::get_builder().SetInsertPoint(bb);

        // record function arguments
        Compiler::Internal::get_named_values().clear();
        for (auto& arg : f->args()) {
            Compiler::Internal::get_named_values()[std::string(arg.getName())] = &arg;
        }

        if (llvm::Value* return_value = body->codegen()) {
            Compiler::Internal::get_builder().CreateRet(return_value);
            llvm::verifyFunction(*f);
            //Optimiser::optimise_function(*f);
            return f;
        }

        f->eraseFromParent();
        return nullptr;
    }

}
