#pragma once

#include "types/data_type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    class StringType: public DataType {
    public:
        std::string get_name() const override;
        llvm::Type* get_llvm_type() const override;

        llvm::Value* cast(llvm::Value* value, TypeToken to) const override;
        llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
        llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const override;
    };

}
