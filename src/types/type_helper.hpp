#pragma once

#include <llvm/IR/Value.h>
#include "type.hpp"

namespace Kepler::Type::Helper {

    llvm::Value* float_to_int_inbounds(llvm::Value* value, TypeToken from, TypeToken to);

}
