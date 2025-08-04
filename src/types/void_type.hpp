#pragma once

#include "types/data_type.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    class VoidType: public DataType {
    public:
        std::string get_name() const override;
        llvm::Type* get_llvm_type() const override;
    };

}
