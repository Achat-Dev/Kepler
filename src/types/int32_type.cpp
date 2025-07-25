#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "int32_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type::Int32Type {

    llvm::Type* get_llvm_type() {
        return llvm::Type::getInt32Ty(Compiler::get_context());
    }

    llvm::Value* cast(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int16: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    std::string get_name() {
        return "i32";
    }

}
