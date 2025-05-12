#include <cassert>
#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>

#include "../compiler.hpp"
#include "../type.hpp"
#include "../utils.hpp"
#include "parameter_data.hpp"
#include "prototype.hpp"

namespace Kepler::AST {

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types;
        for (ParameterData& arg : args) {
            types.push_back(Type::get_by_token(arg.type));
        }
        llvm::FunctionType* ft = llvm::FunctionType::get(Type::get_by_token(type), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, Compiler::get_module());

        unsigned idx = 0;
        for (llvm::Argument& arg : f->args()) {
            arg.setName(args[idx++].name);
        }

        return f;
    }

    TypeToken Prototype::get_type() const {
        return type;
    }

    const std::string& Prototype::get_name() const {
        return name;
    }

    const ParameterData& Prototype::get_arg(int index) const {
        assert(index < args.size() && "[ Assertion ]: trying to get argument at an index which is out of bounds");
        return args[index];
    }

    const ParameterData& Prototype::get_arg(const std::string& name) const {
        for (const ParameterData& data : args) {
            if (data.name == name) {
                return data;
            }
        }

        emergency_exit("failed to find variable '" + name + "' in function arguments");
        return args[0]; // Needed to avoid compile errors
    }

}
