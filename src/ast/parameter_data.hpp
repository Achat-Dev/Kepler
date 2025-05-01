#pragma once

#include <string>

#include "../type.hpp"

namespace Kepler::AST {

    struct ParameterData {
        const TypeToken type;
        const std::string name;

        ParameterData(TypeToken type, std::string name) : type(type), name(std::move(name)) {}
    };

}
