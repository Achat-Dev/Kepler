#pragma once

#include "data_type.hpp"

namespace Kepler::Type {

    class CharType: public DataType {
    public:
        llvm::Type* get_llvm_type() const override;
        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        std::string get_name() const override;
    };

}
