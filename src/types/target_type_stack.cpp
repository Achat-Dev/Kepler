#include <cassert>
#include <stack>

#include "target_type_stack.hpp"
#include "type.hpp"

namespace Kepler::Type::TargetTypeStack {

    static std::stack<Type::TypeToken> target_types;

    void push(Type::TypeToken type) {
        target_types.push(type);
    }

    void pop() {
        return target_types.pop();
    }

    Type::TypeToken top() {
        assert(!target_types.empty() && "[ Assertion ]: peeking at empty TargetTypeStack");
        return target_types.top();
    }

}
