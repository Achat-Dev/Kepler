#include "types/int8_type.hpp"

#include "compiler.hpp"
#include "log.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string Int8Type::get_name() const {
        return "i8";
    }

    llvm::Type* Int8Type::get_llvm_type() const {
        return llvm::Type::getInt8Ty(Compiler::get_context());
    }

    llvm::Value* Int8Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int8), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String: return cast_to_string(value, TypeToken::Int8);
            case TypeToken::Int16: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int32: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Int8Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateAdd(lhs, rhs, "i8_add");
    }

    llvm::Value* Int8Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSub(lhs, rhs, "i8_sub");
    }

    llvm::Value* Int8Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateMul(lhs, rhs, "i8_mul");
    }

    llvm::Value* Int8Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSDiv(lhs, rhs, "i8_div");
    }

    llvm::Value* Int8Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLT(lhs, rhs, "i8_lt");
    }

    llvm::Value* Int8Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGT(lhs, rhs, "i8_gt");
    }

    llvm::Value* Int8Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpEQ(lhs, rhs, "i8_eq");
    }

    llvm::Value* Int8Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpNE(lhs, rhs, "i8_neq");
    }

    llvm::Value* Int8Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLE(lhs, rhs, "i8_leq");
    }

    llvm::Value* Int8Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGE(lhs, rhs, "i8_geq");
    }

}
