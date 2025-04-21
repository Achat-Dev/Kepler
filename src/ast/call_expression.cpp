#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <vector>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "call_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> CallExpression::codegen() {
        llvm::Function* calleef = Compiler::get_module().getFunction(callee);
        if (!calleef) {
            log("Compile error: unknown function called");
            return ExpressionResult::create_invalid();
        }

        if (calleef->arg_size() != args.size()) {
            log("Compile error: incorrect number of arguments passed to function");
            return ExpressionResult::create_invalid();
        }

        std::vector<llvm::Value*> argsv;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            argsv.push_back(args[i]->codegen()->get_value());
            if (!argsv.back()) {
                return ExpressionResult::create_invalid();
            }
        }

        return ExpressionResult::create(Compiler::get_builder().CreateCall(calleef, argsv, "calltmp"), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }


}
