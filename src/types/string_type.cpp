#include "types/string_type.hpp"

#include "ast/call_expression.hpp"
#include "ast/llvm_value_wrapper_expression.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/type_token.hpp"

#include <cassert>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Kepler::Type {

    std::string StringType::get_name() const {
        return "string";
    }

    llvm::Type* StringType::get_llvm_type() const {
        // A string is internally represented as an immutable array of i8
        // However, to get the llvm::Type* of that, the length of the array is needed
        // That's why the type of a string is an i8* (since llvm uses opaque pointers, the pointer is not explicitly typed)
        return llvm::PointerType::get(Compiler::get_context(), 0);
    }

    llvm::Value* StringType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from type 'string' is not supported yet");
        return nullptr;
    }

    llvm::Value* StringType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        const unsigned int flags = AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable;
        std::unique_ptr<AST::LLVMValueWrapperExpression> lhs_expression = std::make_unique<AST::LLVMValueWrapperExpression>(lhs, TypeToken::String, flags);
        std::unique_ptr<AST::LLVMValueWrapperExpression> rhs_expression = std::make_unique<AST::LLVMValueWrapperExpression>(rhs, TypeToken::String, flags);

        std::vector<std::unique_ptr<AST::Expression>> args;
        args.push_back(std::move(lhs_expression));
        args.push_back(std::move(rhs_expression));

        AST::CallExpression string_concat_call_expression = AST::CallExpression("__kepler_string_concat", std::move(args));
        std::unique_ptr<AST::ExpressionResult> string_concat_er = string_concat_call_expression.codegen();

        assert(string_concat_er->is_valid() && "[ Assertion ]: failed to create concatenation of strings, the expression result is invalid");
        assert(string_concat_er->is_assignable() && "[ Assertion ]: failed to create concatenation of strings, the expression result is not assignable");

        return string_concat_er->get_value();
    }

    llvm::Value* StringType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": '==' operation bewteen type 'string' is not supported yet");
        return nullptr;
    }

    llvm::Value* StringType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": '!=' operation bewteen type 'string' is not supported yet");
        return nullptr;
    }

}
