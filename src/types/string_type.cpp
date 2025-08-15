// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/string_type.hpp"

#include "ast/call_expression.hpp"
#include "ast/llvm_value_wrapper_expression.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <cassert>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Kepler::Type {

    static llvm::Value* create_call_to(const std::string& function_name, llvm::Value* lhs, llvm::Value* rhs) {
        const unsigned int flags = AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable;
        std::unique_ptr<AST::LLVMValueWrapperExpression> lhs_expression = std::make_unique<AST::LLVMValueWrapperExpression>(lhs, TypeToken::String, flags);
        std::unique_ptr<AST::LLVMValueWrapperExpression> rhs_expression = std::make_unique<AST::LLVMValueWrapperExpression>(rhs, TypeToken::String, flags);

        std::vector<std::unique_ptr<AST::Expression>> args;
        args.push_back(std::move(lhs_expression));
        args.push_back(std::move(rhs_expression));

        AST::CallExpression call_expression = AST::CallExpression(function_name, std::move(args));
        std::unique_ptr<AST::ExpressionResult> call_er = call_expression.codegen();

        assert(call_er->is_valid() && "[ Assertion ]: failed to create concatenation of strings, the expression result is invalid");
        assert(call_er->is_assignable() && "[ Assertion ]: failed to create concatenation of strings, the expression result is not assignable");

        return call_er->get_value();
    }

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
        return create_call_to("__kepler_string_concat", lhs, rhs);
    }

    llvm::Value* StringType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_less_than(comapre_value, zero, TypeToken::Int32);
    }

    llvm::Value* StringType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_greater_than(comapre_value, zero, TypeToken::Int32);
    }

    llvm::Value* StringType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_equals(comapre_value, zero, TypeToken::Int32);
    }

    llvm::Value* StringType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_not_equals(comapre_value, zero, TypeToken::Int32);
    }

    llvm::Value* StringType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_less_equals(comapre_value, zero, TypeToken::Int32);
    }

    llvm::Value* StringType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        llvm::Value* comapre_value = create_call_to("__kepler_string_compare", lhs, rhs);
        llvm::Value* zero = llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0);
        return Type::create_greater_equals(comapre_value, zero, TypeToken::Int32);
    }

}
