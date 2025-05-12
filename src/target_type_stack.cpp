#include "target_type_stack.hpp"
#include "type.hpp"
#include <cassert>

namespace Kepler {

    void TargetTypeStack::push(Type::TypeToken type) {
        target_types.push(type);
    }

    void TargetTypeStack::pop() {
        return target_types.pop();
    }

    Type::TypeToken TargetTypeStack::top() const {
        assert(!target_types.empty() && "[ Assertion ]: peeking at empty TargetTypeStack");
        return target_types.top();
    }

}
