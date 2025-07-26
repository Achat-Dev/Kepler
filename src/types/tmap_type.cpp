#include <exception>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"
#include "tmap_type.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    std::string TMapType::get_name() const {
        return "tmap";
    }

    llvm::Type* TMapType::get_llvm_type() const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'tmap' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* TMapType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'tmap' cannot be casted");
        return nullptr;
    }

    llvm::Value* TMapType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* TMapType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

}
