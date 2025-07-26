#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "int64_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    std::string Int64Type::get_name() const {
        return "i64";
    }

    llvm::Type* Int64Type::get_llvm_type() const {
        return llvm::Type::getInt64Ty(Compiler::get_context());
    }

    llvm::Value* Int64Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int64), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int16: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int32: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Int64Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateAdd(lhs, rhs, "i64_add");
    }

    llvm::Value* Int64Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSub(lhs, rhs, "i64_sub");
    }

    llvm::Value* Int64Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateMul(lhs, rhs, "i64_mul");
    }

    llvm::Value* Int64Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateSDiv(lhs, rhs, "i64_div");
    }

    llvm::Value* Int64Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLT(lhs, rhs, "i64_lt");
    }

    llvm::Value* Int64Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGT(lhs, rhs, "i64_gt");
    }

    llvm::Value* Int64Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpEQ(lhs, rhs, "i64_eq");
    }

    llvm::Value* Int64Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpNE(lhs, rhs, "i64_neq");
    }

    llvm::Value* Int64Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSLE(lhs, rhs, "i64_leq");
    }

    llvm::Value* Int64Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateICmpSGE(lhs, rhs, "i64_geq");
    }

}
