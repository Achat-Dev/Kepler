#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <vector>

#include "../compiler.hpp"
#include "../log.hpp"
#include "call_expression.hpp"

namespace Kepler::AST {

    llvm::Value* CallExpression::codegen() {
        llvm::Function* calleef = Compiler::Internal::get_module().getFunction(callee);
        if (!calleef) {
            return log_errorv("unknown function called");
        }

        if (calleef->arg_size() != args.size()) {
            return log_errorv("incorrect number of arguments passed to function");
        }

        std::vector<llvm::Value*> argsv;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            argsv.push_back(args[i]->codegen());
            if (!argsv.back()) {
                return nullptr;
            }
        }

        return Compiler::Internal::get_builder().CreateCall(calleef, argsv, "calltmp");
    }


}
