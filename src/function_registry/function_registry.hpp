// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/prototype.hpp"

#include <memory>
#include <string>

namespace Kepler::FunctionRegistry {

    bool register_prototype(std::shared_ptr<AST::Prototype> prototype);
    std::shared_ptr<AST::Prototype> get_registered_prototype(const std::string& name);
    std::shared_ptr<AST::Prototype> get_current_prototype();
    void set_current_prototype(std::shared_ptr<AST::Prototype> prototype);

}
