#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../optimiser.hpp"
#include "../utils.hpp"
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
            log("Compile error: failed to compile function prototype");
            return nullptr;
        }

        if (!f->empty()) {
            log("Compile error: function cannot be redefined");
            return nullptr;
        }

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(Compiler::get_context(), "entry", f);
        Compiler::get_builder().SetInsertPoint(bb);

        // record function arguments
        Compiler::get_named_values().clear();
        for (auto& arg : f->args()) {
            llvm::AllocaInst* alloca = create_entry_block_alloca(f, arg.getName());
            Compiler::get_builder().CreateStore(&arg, alloca);
            Compiler::get_named_values()[std::string(arg.getName())] = alloca;
        }

        if (auto return_value = body->codegen()) {
            Compiler::get_builder().CreateRet(return_value->get_value());
            llvm::verifyFunction(*f);
            //Optimiser::optimise_function(*f);
            return f;
        }

        f->eraseFromParent();
        return nullptr;
    }

}
