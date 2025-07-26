#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "int8_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* Int8Type::get_llvm_type() const {
        return llvm::Type::getInt8Ty(Compiler::get_context());
    }

    llvm::Value* Int8Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int8), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int16: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int32: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    std::string Int8Type::get_name() const {
        return "i8";
    }

}
