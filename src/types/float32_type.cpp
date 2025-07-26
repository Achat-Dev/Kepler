#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "float32_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* Float32Type::get_llvm_type() const {
        return llvm::Type::getFloatTy(Compiler::get_context());
    }

    llvm::Value* Float32Type::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float32), 0));
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int16: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int32: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Int64: return float_to_int_inbounds(value, TypeToken::Float32, to);
            case TypeToken::Float64: return Compiler::get_builder().CreateFPExt(value, get_by_token(to));
            default: return nullptr;
        }
    }

    std::string Float32Type::get_name() const {
        return "f32";
    }

}
