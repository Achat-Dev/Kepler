#pragma once

#include "types/type_token.hpp"

namespace Kepler::Type::TargetTypeStack {

    void push(TypeToken type);
    void pop();
    TypeToken top();

}
