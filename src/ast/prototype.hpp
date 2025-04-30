#pragma once

#include <llvm/IR/Function.h>
#include <string>
#include <vector>

#include "../type.hpp"

namespace Kepler::AST {

    class Prototype {
    private:
        TypeToken type;
        std::string name;
        std::vector<std::string> args;

    public:
        Prototype() : name(""), args(std::vector<std::string>()) {}
        Prototype(TypeToken type, std::string name, std::vector<std::string> args)
            : type(type), name(name), args(std::move(args)) {}
        llvm::Function* codegen();
        const std::string& get_name() const { return name; }
    };


}
