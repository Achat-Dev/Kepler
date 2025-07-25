#include <cassert>
#include <limits>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>

#include "type_helper.hpp"
#include "type.hpp"
#include "../compiler.hpp"

namespace Kepler::Type::Helper {

    llvm::Value* float_to_int_inbounds(llvm::Value* value, TypeToken from, TypeToken to) {
        assert((Type::is_floating_point_type(from) && Type::is_integer_type(to)) && "[ Assertion ]: TypeTokens given to 'float_to_int' are not of types floating point and integer");

        llvm::Value* min;
        llvm::Value* max;

        switch (to) {
            case Type::TypeToken::Int8:
                min = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int8_t>::lowest());
                max = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int8_t>::max());
                break;
            case Type::TypeToken::Int16:
                min = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int16_t>::lowest());
                max = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int16_t>::max());
                break;
            case Type::TypeToken::Int32:
                min = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int32_t>::lowest());
                max = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int32_t>::max());
                break;
            case Type::TypeToken::Int64:
                min = llvm::ConstantFP::get(Type::get_by_token(from), std::numeric_limits<int64_t>::lowest());
                max = llvm::ConstantFP::get(Type::get_by_token(from), static_cast<double>(std::numeric_limits<int64_t>::max()));
                break;
            default: break;
        }

        llvm::Value* clamped = Compiler::get_builder().CreateBinaryIntrinsic(llvm::Intrinsic::maxnum, value, min);
        clamped = Compiler::get_builder().CreateBinaryIntrinsic(llvm::Intrinsic::minnum, clamped, max);

        return Compiler::get_builder().CreateFPToSI(clamped, Type::get_by_token(to));
    }

}
