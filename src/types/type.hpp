#pragma once

#include "types/type_token.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    bool is_floating_point_type(TypeToken type);
    bool is_integer_type(TypeToken type);
    std::string get_type_name(TypeToken type);
    llvm::Type* get_by_token(TypeToken type);

    llvm::Value* cast(llvm::Value* value, TypeToken from, TypeToken to);
    bool create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data);
    bool create_assign(llvm::Value* value, TypeToken value_type, const LocalVariables::VariableData& variable_data);
    llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);
    llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type);

}
