#pragma once

#include <memory>
#include <string>

#include "../ast/prototype.hpp"

namespace Kepler::FunctionRegistry {

    bool register_prototype(std::shared_ptr<AST::Prototype> prototype);
    std::shared_ptr<AST::Prototype> get_registered_prototype(const std::string& name);
    std::shared_ptr<AST::Prototype> get_compiling_prototype();
    void set_compiling_prototype(std::shared_ptr<AST::Prototype> prototype);

}
