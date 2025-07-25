#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "float64_type.hpp"
#include "type.hpp"
#include "type_helper.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type::Float64Type {

    llvm::Type* get_llvm_type() {
        return llvm::Type::getDoubleTy(Compiler::get_context());
    }

    llvm::Value* cast(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool: return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float64), 0));
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8: return Helper::float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int16: return Helper::float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int32: return Helper::float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Int64: return Helper::float_to_int_inbounds(value, TypeToken::Float64, to);
            case TypeToken::Float32: return Compiler::get_builder().CreateFPTrunc(value, get_by_token(to));
            default: return nullptr;
        }
    }

    std::string get_name() {
        return "f64";
    }

}
