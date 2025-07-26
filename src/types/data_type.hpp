#pragma once

#include "type.hpp"
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>
namespace Kepler::Type {

    class DataType {
    public:
        virtual ~DataType() = default;
        virtual llvm::Type* get_llvm_type() const = 0;
        virtual llvm::Value* cast(llvm::Value* value, TypeToken to) const = 0;
        virtual std::string get_name() const = 0;
    };

}
