#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "floating_point_type.hpp"
#include "type.hpp"

namespace Kepler::Type {

    class Float64Type: public FloatingPointType {
    public:
        llvm::Type* get_llvm_type() const override;
        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        std::string get_name() const override;
    };

}
