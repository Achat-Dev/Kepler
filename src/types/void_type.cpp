#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"
#include "void_type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* VoidType::get_llvm_type() const {
        return llvm::Type::getVoidTy(Compiler::get_context());
    }

    llvm::Value* VoidType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'void' cannot be casted");
        return nullptr;
    }

    std::string VoidType::get_name() const {
        return "void";
    }

}
