#pragma once

#include <ostream>

namespace Kepler::Type {

    enum class TypeToken {
        None,
        Void,
        TMap,
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

}
