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
