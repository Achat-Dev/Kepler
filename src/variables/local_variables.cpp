// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "variables/local_variables.hpp"

#include "log.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Instructions.h>
#include <map>
#include <optional>

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
