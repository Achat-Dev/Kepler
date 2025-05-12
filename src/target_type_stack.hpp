#pragma once

#include <stack>

#include "type.hpp"

namespace Kepler {

    class TargetTypeStack {
    private:
        std::stack<Type::TypeToken> target_types;

    public:
        void push(Type::TypeToken type);
        void pop();
        Type::TypeToken top() const;
    };

}
