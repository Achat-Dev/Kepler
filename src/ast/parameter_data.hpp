#pragma once

#include "types/type_token.hpp"

#include <string>

namespace Kepler::AST {

    struct ParameterData {
        const Type::TypeToken type;
        const std::string name;

        ParameterData(Type::TypeToken type, std::string name) : type(type), name(std::move(name)) {}
    };

}
