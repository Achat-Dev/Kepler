#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <vector>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../type.hpp"
#include "prototype.hpp"

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        // Temporary until all data types are implemented
        llvm::Type* llvm_type = Type::get_by_token(type);
        if (!llvm_type) {
            log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type '", (int)type, "' not supported yet");
            return nullptr;
        }

        std::vector<llvm::Type*> types(args.size(), llvm::Type::getDoubleTy(Compiler::get_context()));
        llvm::FunctionType* ft = llvm::FunctionType::get(Type::get_by_token(type), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned idx = 0;
        for (auto& arg : f->args()) {
            arg.setName(args[idx++]);
        }

        return f;
    }

}
