#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <vector>

#include "call_expression.hpp"
#include "expression_result.hpp"
#include "parameter_data.hpp"
#include "prototype.hpp"
#include "../compiler.hpp"
#include "../function_registry/function_registry.hpp"
#include "../log.hpp"
#include "../types/target_type_stack.hpp"

namespace Kepler::AST {

    // TODO: error message when type of argument doesn't match expected type
    std::unique_ptr<ExpressionResult> CallExpression::codegen() {
        llvm::Function* callee_f = Compiler::get_module().getFunction(callee);
        if (!callee_f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown function '", callee, "' called");
            return ExpressionResult::create_invalid();
        }

        if (callee_f->arg_size() != args.size()) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": incorrect number of arguments passed to function");
            return ExpressionResult::create_invalid();
        }

        std::shared_ptr<Prototype> prototype = FunctionRegistry::get_registered_prototype(callee);
        assert(prototype != nullptr && "[ Assertion ]: protoype called from CallExpression not registered as known prototype");

        const std::vector<ParameterData>& parameters = prototype->get_parameters();
        assert(args.size() == parameters.size() && "[ Assertion ]: argument count of CallExpression and respective prototype don't match");

        std::vector<llvm::Value*> args_v;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            Type::TargetTypeStack::push(parameters[i].type);
            std::unique_ptr<ExpressionResult> arg_er = args[i]->codegen();
            if (!arg_er->is_valid()) {
                log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": invalid expression in function argument");
                return ExpressionResult::create_invalid();
            }

            args_v.push_back(arg_er->get_value());
            if (!args_v.back()) {
                return ExpressionResult::create_invalid();
            }
            Type::TargetTypeStack::pop();
        }

        if (prototype->get_type() == Type::TypeToken::Void) {
            return ExpressionResult::create(Compiler::get_builder().CreateCall(callee_f, args_v), prototype->get_type(), ExpressionResultFlags::Valid);
        }
        return ExpressionResult::create(Compiler::get_builder().CreateCall(callee_f, args_v, "calltmp"), prototype->get_type(), ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }


}
