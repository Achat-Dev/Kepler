#pragma once

#include <llvm/IR/Function.h>
#include <string>
#include <vector>

namespace Kepler::AST {

    class Prototype {
    private:
        std::string name;
        std::vector<std::string> args;

    public:
        Prototype() : name(""), args(std::vector<std::string>()) {}
        Prototype(const std::string& name, std::vector<std::string> args)
            : name(name), args(std::move(args)) {}
        llvm::Function* codegen();
        const std::string& get_name() const { return name; }
    };


}
