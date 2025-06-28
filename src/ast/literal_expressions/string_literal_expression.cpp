#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <memory>

#include "string_literal_expression.hpp"
#include "../expression_result.hpp"
#include "../../compiler.hpp"
#include "../../log.hpp"
#include "../../types/target_type_stack.hpp"
#include "../../types/type.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> StringLiteralExpression::codegen() {
        Type::TypeToken type = Type::TargetTypeStack::top();

        if (type != Type::TypeToken::String && type != Type::TypeToken::None) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a string literal");
            return ExpressionResult::create_invalid();
        }

        // Constant array that holds the string data
        llvm::Constant* data = llvm::ConstantDataArray::getString(Compiler::get_context(), value);

        // i8* that points to the data
        llvm::GlobalVariable* variable = new llvm::GlobalVariable(Compiler::get_module(), data->getType(), true, llvm::GlobalValue::PrivateLinkage, data);

        // Optimisation: tell llvm that the pointer is never going to be compared
        // (only the value of the string is going to be compared, never the reference to the string)
        variable->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        return ExpressionResult::create(variable, Type::TypeToken::String, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
