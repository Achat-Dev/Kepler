#include "types/target_type_stack.hpp"

#include "types/type_token.hpp"

#include <cassert>
#include <stack>

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
