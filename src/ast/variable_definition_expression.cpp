#include <cassert>
#include <llvm/IR/Function.h>
#include <memory>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../utils.hpp"
#include "expression_result.hpp"
#include "variable_definition_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableDefinitionExpression::codegen() {
        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();
        if (!f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable defined outside of function");
            return ExpressionResult::create_invalid();
        }

        if (Compiler::get_named_values()[name]) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable '", name, "' already exists");
            return ExpressionResult::create_invalid();
        }

        llvm::AllocaInst* alloca = create_entry_block_alloca(f, name);
        Compiler::get_named_values()[std::string(name)] = alloca;

        assert(value->get_operator() == '=' && "Operator of variable assignment has to be '='");

        // Since this is a BinaryExpression codegen handles the assignment and error handling
        std::unique_ptr<ExpressionResult> valuev = value->codegen();

        return std::move(valuev);
    }

}
