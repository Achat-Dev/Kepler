// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/parameter_data.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Function.h>
#include <string>
#include <vector>

namespace Kepler::AST {

    class Prototype {
    private:
        Type::TypeToken type;
        std::string name;
        std::vector<ParameterData> parameters;

    public:
        Prototype() : name(""), parameters(std::vector<ParameterData>()) {}
        Prototype(Type::TypeToken type, std::string name, std::vector<ParameterData> parameters)
            : type(type), name(name), parameters(std::move(parameters)) {}
        llvm::Function* codegen();
        Type::TypeToken get_type() const;
        const std::string& get_name() const;
        const ParameterData& get_parameter(const std::string& name) const;
        const std::vector<ParameterData>& get_parameters() const;
    };


}
