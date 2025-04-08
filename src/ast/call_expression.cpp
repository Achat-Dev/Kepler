#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <vector>

#include "../compiler.hpp"
#include "../log.hpp"
#include "call_expression.hpp"

namespace Kepler::AST {

    llvm::Value* CallExpression::codegen() {
        llvm::Function* calleef = Compiler::get_module().getFunction(callee);
        if (!calleef) {
            log("Compile error: unknown function called");
            return nullptr;
        }

        if (calleef->arg_size() != args.size()) {
            log("Compile error: incorrect number of arguments passed to function");
            return nullptr;
        }

        std::vector<llvm::Value*> argsv;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            argsv.push_back(args[i]->codegen());
            if (!argsv.back()) {
                return nullptr;
            }
        }

        return Compiler::get_builder().CreateCall(calleef, argsv, "calltmp");
    }


}
