#pragma once

#include "types/type_token.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    class DataType {
    public:
        virtual ~DataType() = default;
        virtual std::string get_name() const = 0;
        virtual llvm::Type* get_llvm_type() const = 0;

        virtual llvm::Value* cast(llvm::Value* value, TypeToken to) const;
        virtual void create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data) const;
        virtual llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const;
        virtual llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const;

    protected:
        llvm::Value* cast_to_string(llvm::Value* value, TypeToken from) const;
    };

}
