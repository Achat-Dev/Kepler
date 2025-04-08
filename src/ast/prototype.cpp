#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <vector>

#include "../compiler.hpp"
#include "prototype.hpp"

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types(args.size(), llvm::Type::getDoubleTy(Compiler::get_context()));
        llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getDoubleTy(Compiler::get_context()), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned idx = 0;
        for (auto& arg : f->args()) {
            arg.setName(args[idx++]);
        }

        return f;
    }

}
