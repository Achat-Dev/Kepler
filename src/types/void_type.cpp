#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"
#include "void_type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    std::string VoidType::get_name() const {
        return "void";
    }

    llvm::Type* VoidType::get_llvm_type() const {
        return llvm::Type::getVoidTy(Compiler::get_context());
    }

    llvm::Value* VoidType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'void' cannot be casted");
        return nullptr;
    }

    llvm::Value* VoidType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* VoidType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

}
