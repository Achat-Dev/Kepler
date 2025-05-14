#include <cassert>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <string>

#include "expression_result.hpp"
#include "variable_definition_expression.hpp"
#include "../compiler.hpp"
#include "../log.hpp"
#include "../types/type.hpp"
#include "../utils.hpp"
#include "../variables/local_variables.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableDefinitionExpression::codegen() {
        llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();
        if (!f) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable defined outside of function");
            return ExpressionResult::create_invalid();
        }

        if (LocalVariables::get(name)) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": local variable '", name, "' already exists");
            return ExpressionResult::create_invalid();
        }

        llvm::AllocaInst* alloca = create_entry_block_alloca(f, Type::get_by_token(type), name);
        LocalVariables::set(name, { type, alloca });

        assert(value->get_operator() == '=' && "[ Assertion ]: operator of variable assignment has to be '='");

        // Since this is a BinaryExpression, codegen handles the assignment and error handling
        std::unique_ptr<ExpressionResult> value_er = value->codegen();

        return std::move(value_er);
    }

    const std::string& VariableDefinitionExpression::get_name() const {
        return name;
    }

}
