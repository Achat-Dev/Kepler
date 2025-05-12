#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <vector>

#include "../compiler.hpp"
#include "../log.hpp"
#include "expression_result.hpp"
#include "prototype.hpp"
#include "call_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> CallExpression::codegen() {
        llvm::Function* callee_f = Compiler::get_module().getFunction(callee);
        if (!callee_f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown function called");
            return ExpressionResult::create_invalid();
        }

        if (callee_f->arg_size() != args.size()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": incorrect number of arguments passed to function");
            return ExpressionResult::create_invalid();
        }

        std::shared_ptr<Prototype> prototype = Compiler::get_prototypes()[callee];
        assert(prototype != nullptr && "[ Assertion ]: protoype called from CallExpression not added to known prototypes");

        std::vector<llvm::Value*> args_v;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            Compiler::get_target_type_stack().push(prototype->get_arg(i).type);
            std::unique_ptr<ExpressionResult> arg_er = args[i]->codegen();
            if (!arg_er) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression for function call argument");
                return ExpressionResult::create_invalid();
            }

            args_v.push_back(arg_er->get_value());
            if (!args_v.back()) {
                return ExpressionResult::create_invalid();
            }
            Compiler::get_target_type_stack().pop();
        }

        return ExpressionResult::create(Compiler::get_builder().CreateCall(callee_f, args_v, "calltmp"), prototype->get_type(), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }


}
