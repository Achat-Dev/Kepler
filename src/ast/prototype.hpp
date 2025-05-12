#pragma once

#include <llvm/IR/Function.h>
#include <string>
#include <vector>

#include "../type.hpp"
#include "parameter_data.hpp"

namespace Kepler::AST {

    class Prototype {
    private:
        TypeToken type;
        std::string name;
        std::vector<ParameterData> args;

    public:
        Prototype() : name(""), args(std::vector<ParameterData>()) {}
        Prototype(TypeToken type, std::string name, std::vector<ParameterData> args)
            : type(type), name(name), args(std::move(args)) {}
        llvm::Function* codegen();
        TypeToken get_type() const;
        const std::string& get_name() const;
        const ParameterData& get_arg(int index) const;
        const ParameterData& get_arg(const std::string& name) const;
    };


}
