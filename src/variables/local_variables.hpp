#pragma once

#include <optional>
#include <string>

#include "variable_data.hpp"

namespace Kepler::LocalVariables {

    std::optional<VariableData> get(const std::string& name);
    void set(const std::string& name, const VariableData& data);
    void update(const std::string& name, const VariableData& data);
    void erase(const std::string& name);
    void clear();

}
