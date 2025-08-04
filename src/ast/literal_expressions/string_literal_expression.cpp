#include "ast/literal_expressions/string_literal_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> StringLiteralExpression::codegen() {
        Type::TypeToken type = Type::TargetTypeStack::top();

        if (type != Type::TypeToken::String && type != Type::TypeToken::None && type != Type::TypeToken::TMap) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a string literal");
            return ExpressionResult::create_invalid();
        }

        // Constant array that holds the string data (null terminated)
        llvm::Constant* data = llvm::ConstantDataArray::getString(Compiler::get_context(), value, true);

        // Pointer that points to the constant array
        llvm::GlobalVariable* variable = new llvm::GlobalVariable(Compiler::get_module(), data->getType(), true, llvm::GlobalValue::PrivateLinkage, data);

        // Optimisation: tell llvm that the pointer is never going to be compared
        // (only the value of the string is going to be compared, never the reference to the string)
        variable->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        // Create i8* to the first element of the constant array
        llvm::Constant* zero = llvm::ConstantInt::get(Type::get_by_token(Type::TypeToken::Int32), 0);
        llvm::Constant* indices[] = { zero, zero };
        llvm::Constant* value = llvm::ConstantExpr::getInBoundsGetElementPtr(data->getType(), variable, indices);

        return ExpressionResult::create(value, Type::TypeToken::String, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
