#pragma once

// Idea:
// Explicit types are only needed when:
// - a variable is defined
// - a function is defined
// - something is casted
//
// The parser just checks if there is a data type and save the current data type
// Depending on what comes after the data type it handles either variable definitions, functions or casts
// - <data_type> identifier (...) -> function
// - <data_type> identifier = ... -> variable
// - <data_type>(...) -> cast
//
// When compiling, the codegened value has a llvm type (expression_result->get_value()->getType())
// When expressions need type safety, compare the llvm type of the expression with the desired llvm type
// - Type class with static method 'get_llvm_type(TypeToken)'

#include <llvm/IR/Type.h>

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

    std::ostream& operator<<(std::ostream& os, const TypeToken& color);

    llvm::Type* get_by_token(TypeToken type);

}
