#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "int16_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* Int16Type::get_llvm_type() const {
        return llvm::Type::getInt16Ty(Compiler::get_context());
    }

    llvm::Value* Int16Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int16), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8: return Compiler::get_builder().CreateTrunc(value, get_by_token(to));
            case TypeToken::Int32: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Int64: return Compiler::get_builder().CreateSExt(value, get_by_token(to));
            case TypeToken::Float32: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            case TypeToken::Float64: return Compiler::get_builder().CreateSIToFP(value, get_by_token(to));
            default: return nullptr;
        }
    }

    std::string Int16Type::get_name() const {
        return "i16";
    }

}
