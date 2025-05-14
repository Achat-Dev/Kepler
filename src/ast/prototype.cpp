#include <cassert>
#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>

#include "parameter_data.hpp"
#include "prototype.hpp"
#include "../compiler.hpp"
#include "../types/type.hpp"
#include "../utils.hpp"

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types;
        for (ParameterData& parameter : parameters) {
            types.push_back(Type::get_by_token(parameter.type));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(Type::get_by_token(type), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned idx = 0;
        for (llvm::Argument& arg : f->args()) {
            arg.setName(parameters[idx++].name);
        }

        return f;
    }

    Type::TypeToken Prototype::get_type() const {
        return type;
    }

    const std::string& Prototype::get_name() const {
        return name;
    }

    const ParameterData& Prototype::get_parameter(const std::string& name) const {
        for (const ParameterData& parameter : parameters) {
            if (parameter.name == name) {
                return parameter;
            }
        }

        emergency_exit("failed to find variable '" + name + "' in function arguments");
        return parameters[0]; // Needed to avoid compile errors
    }

    const std::vector<ParameterData>& Prototype::get_parameters() const {
        return parameters;
    }

}
