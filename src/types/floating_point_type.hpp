#pragma once

#include <llvm/IR/Value.h>

#include "data_type.hpp"
#include "type.hpp"

namespace Kepler::Type {

    class FloatingPointType: public DataType {
    protected:
        llvm::Value* float_to_int_inbounds(llvm::Value* value, TypeToken from, TypeToken to) const;
    };

}
