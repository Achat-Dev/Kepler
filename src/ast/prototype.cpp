#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <vector>

#include "../compiler.hpp"
#include "../type.hpp"
#include "parameter_data.hpp"
#include "prototype.hpp"

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types;
        for (ParameterData& arg : args) {
            types.push_back(Type::get_by_token(arg.type));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(Type::get_by_token(type), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned idx = 0;
        for (llvm::Argument& arg : f->args()) {
            arg.setName(args[idx++].name);
        }

        return f;
    }

}
