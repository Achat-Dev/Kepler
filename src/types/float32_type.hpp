#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "floating_point_type.hpp"
#include "type.hpp"

namespace Kepler::Type {

    class Float32Type: public FloatingPointType {
    public:
        std::string get_name() const override;
        llvm::Type* get_llvm_type() const override;

        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
    };

}
