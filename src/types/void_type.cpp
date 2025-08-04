#include "types/void_type.hpp"

#include "compiler.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string VoidType::get_name() const {
        return "void";
    }

    llvm::Type* VoidType::get_llvm_type() const {
        return llvm::Type::getVoidTy(Compiler::get_context());
    }

}
