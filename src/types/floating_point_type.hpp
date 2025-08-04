#pragma once

#include "types/data_type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Value.h>

namespace Kepler::Type {

    class FloatingPointType: public DataType {
    protected:
        llvm::Value* float_to_int_inbounds(llvm::Value* value, TypeToken from, TypeToken to) const;
    };

}
