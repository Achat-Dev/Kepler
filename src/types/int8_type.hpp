#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "data_type.hpp"
#include "type.hpp"

namespace Kepler::Type {

    class Int8Type: public DataType {
    public:
        llvm::Type* get_llvm_type() const override;
        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        std::string get_name() const override;
    };

}
