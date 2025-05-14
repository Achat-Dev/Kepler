#pragma once

#include <llvm/IR/Type.h>
#include <string>

namespace Kepler::Type {

    enum class TypeToken {
        None,
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
    std::string to_string(TypeToken type);

}
