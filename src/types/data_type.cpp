// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/data_type.hpp"

#include "ast/call_expression.hpp"
#include "ast/expression_result.hpp"
#include "ast/llvm_value_wrapper_expression.hpp"
#include "compiler.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"
#include "variables/variable_data.hpp"

#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

namespace Kepler::Type {

    llvm::Value* DataType::cast(llvm::Value* value, TypeToken to) const {
        return nullptr;
    }

    bool DataType::create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data) const {
        Compiler::get_builder().CreateStore(value, variable_data.variable);
        return true;
    }

    bool DataType::create_assign(llvm::Value* value, TypeToken value_type, const LocalVariables::VariableData& variable_data) const {
        return false;
    }

    llvm::Value* DataType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        return nullptr;
    }

    llvm::Value* DataType::cast_to_string(llvm::Value* value, TypeToken from) const {
        const unsigned int flags = AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable;
        std::unique_ptr<AST::LLVMValueWrapperExpression> arg_expression = std::make_unique<AST::LLVMValueWrapperExpression>(value, from, flags);
        std::vector<std::unique_ptr<AST::Expression>> args;
        args.push_back(std::move(arg_expression));

        std::string from_name = get_type_name(from);
        AST::CallExpression call_expression = AST::CallExpression("__kepler_" + from_name + "_to_string", std::move(args));
        std::unique_ptr<AST::ExpressionResult> call_er = call_expression.codegen();

        assert(call_er->is_valid() && "[ Assertion ]: failed to cast" && from_name.c_str() && "to string, the expression result is invalid");
        assert(call_er->is_assignable() && "[ Assertion ]: failed to cast" && from_name.c_str() && "to string, the expression result is not assignable");

        return call_er->get_value();
    }

}
