#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    enum class TypeToken {
        None,
        Void,
        Var,
        Bool,
        Char,
        String,
        Int8,
        Int16,
        Int32,
        Int64,
        Float32,
        Float64
    };

    std::ostream& operator<<(std::ostream& os, TypeToken type);

    llvm::Type* get_by_token(TypeToken type);
    llvm::Value* cast(llvm::Value* value, TypeToken from, TypeToken to);
    std::string to_string(TypeToken type);
    bool is_floating_point_type(TypeToken type);
    bool is_integer_type(TypeToken type);

}
