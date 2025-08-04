#include "types/float64_type.hpp"

#include "compiler.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string Float64Type::get_name() const {
        return "f64";
    }

    llvm::Type* Float64Type::get_llvm_type() const {
        return llvm::Type::getDoubleTy(Compiler::get_context());
    }

    llvm::Value* Float64Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float64), 0));
            case TypeToken::String: return cast_to_string(value, TypeToken::Float64);
            case TypeToken::Int8: return float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int16: return float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int32: return float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int64: return float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Float32: return Compiler::get_builder().CreateFPTrunc(value, get_by_token(to));
            default: return nullptr;
        }
    }

    llvm::Value* Float64Type::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFAdd(lhs, rhs, "f64_add");
    }

    llvm::Value* Float64Type::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFSub(lhs, rhs, "f64_sub");
    }

    llvm::Value* Float64Type::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFMul(lhs, rhs, "f64_mul");
    }

    llvm::Value* Float64Type::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFDiv(lhs, rhs, "f64_div");
    }

    llvm::Value* Float64Type::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpULT(lhs, rhs, "f64_lt");
    }

    llvm::Value* Float64Type::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUGT(lhs, rhs, "f64_gt");
    }

    llvm::Value* Float64Type::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUEQ(lhs, rhs, "f64_eq");
    }

    llvm::Value* Float64Type::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUNE(lhs, rhs, "f64_neq");
    }

    llvm::Value* Float64Type::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpULE(lhs, rhs, "f64_leq");
    }

    llvm::Value* Float64Type::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return Compiler::get_builder().CreateFCmpUGE(lhs, rhs, "f64_geq");
    }

}
