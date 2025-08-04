#include "types/type_token.hpp"

#include "types/type.hpp"

#include <ostream>

namespace Kepler::Type {

    std::ostream& operator<<(std::ostream& os, TypeToken type) {
        os << get_type_name(type);
        return os;
    }

}
