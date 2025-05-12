#pragma once

#include <stack>

#include "type.hpp"

namespace Kepler {

    class TargetTypeStack {
    private:
        std::stack<TypeToken> target_types;

    public:
        void push(TypeToken type);
        void pop();
        TypeToken top() const;
    };

}
