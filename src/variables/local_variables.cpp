#include <llvm/IR/Instructions.h>
#include <map>
#include <optional>

#include "../log.hpp"
#include "local_variables.hpp"
#include "variable_data.hpp"

namespace Kepler::LocalVariables {

    static std::map<std::string, VariableData> local_variables;

    std::optional<VariableData> get(const std::string& name) {
        if (local_variables.find(name) != local_variables.end()) {
            VariableData& data = local_variables[name];
            return VariableData{ data.type, data.variable };
        }
        return std::nullopt;
    }

    void set(const std::string& name, const VariableData& data){
        local_variables[name] = VariableData{ data.type, data.variable };
    }

    void update(const std::string& name, const VariableData& data){
        if (local_variables.find(name) != local_variables.end()) {
            VariableData& d = local_variables[name];
            d.type = data.type;
            d.variable = data.variable;
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to update local variable '", name, '\'');
    }

    void erase(const std::string& name) {
        local_variables.erase(name);
    }

    void clear() {
        local_variables.clear();
    }

}
