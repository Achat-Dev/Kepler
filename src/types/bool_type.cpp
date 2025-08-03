#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "bool_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"

namespace Kepler::Type {

    std::string BoolType::get_name() const {
        return "bool";
    }

    llvm::Type* BoolType::get_llvm_type() const {
        return llvm::Type::getInt1Ty(Compiler::get_context());
    }

    llvm::Value* BoolType::cast(llvm::Value* value, TypeToken to) const {
        if (to == TypeToken::String) {
            return cast_to_string(value, TypeToken::Bool);
        }
        return nullptr;
    }

    llvm::Value* BoolType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpEQ(lhs, rhs, "bool_eq");
    }

    llvm::Value* BoolType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpNE(lhs, rhs, "bool_neq");
    }

    llvm::Value* BoolType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* BoolType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

}
