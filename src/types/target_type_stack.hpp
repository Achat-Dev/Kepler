#pragma once

#include "type.hpp"

namespace Kepler::Type::TargetTypeStack {

    void push(TypeToken type);
    void pop();
    TypeToken top();

}
