#include "ast/prototype.hpp"

#include "ast/parameter_data.hpp"
#include "compiler.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"
#include "utils.hpp"

#include <cassert>
#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types;
        for (ParameterData& parameter : parameters) {
            types.push_back(Type::get_by_token(parameter.type));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(Type::get_by_token(type), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned int index = 0;
        for (llvm::Argument& arg : f->args()) {
            arg.setName(parameters[index++].name);
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
