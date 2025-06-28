#include <llvm/IR/Instructions.h>
#include <map>
#include <optional>

#include "local_variables.hpp"
#include "variable_data.hpp"
#include "../log.hpp"

namespace Kepler::LocalVariables {

    static std::map<std::string, VariableData> local_variables;

    std::optional<VariableData> get(const std::string& name) {
        if (local_variables.find(name) != local_variables.end()) {
            VariableData& local_variable = local_variables[name];
            return VariableData{ local_variable.type, local_variable.variable };
        }
        return std::nullopt;
    }

    void set(const std::string& name, const VariableData& data){
        local_variables[name] = VariableData{ data.type, data.variable };
    }

    void update(const std::string& name, const VariableData& data){
        if (local_variables.find(name) == local_variables.end()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": failed to update local variable '", name, '\'');
            return;
        }

        VariableData& local_variable = local_variables[name];
        local_variable.type = data.type;
        local_variable.variable = data.variable;
    }

    void erase(const std::string& name) {
        local_variables.erase(name);
    }

    void clear() {
        local_variables.clear();
    }

}
