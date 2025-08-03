#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "int32_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    std::string Int32Type::get_name() const {
        return "i32";
    }

    llvm::Type* Int32Type::get_llvm_type() const {
        return llvm::Type::getInt32Ty(Compiler::get_context());
    }

    llvm::Value* Int32Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String: return cast_to_string(value, TypeToken::Int32);
            case TypeToken::Int8: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int16: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Int32Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateAdd(lhs, rhs, "i32_add");
    }

    llvm::Value* Int32Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSub(lhs, rhs, "i32_sub");
    }

    llvm::Value* Int32Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateMul(lhs, rhs, "i32_mul");
    }

    llvm::Value* Int32Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSDiv(lhs, rhs, "i32_div");
    }

    llvm::Value* Int32Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLT(lhs, rhs, "i32_lt");
    }

    llvm::Value* Int32Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGT(lhs, rhs, "i32_gt");
    }

    llvm::Value* Int32Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpEQ(lhs, rhs, "i32_eq");
    }

    llvm::Value* Int32Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpNE(lhs, rhs, "i32_neq");
    }

    llvm::Value* Int32Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLE(lhs, rhs, "i32_leq");
    }

    llvm::Value* Int32Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGE(lhs, rhs, "i32_geq");
    }

}
