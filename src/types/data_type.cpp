#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

#include "data_type.hpp"
#include "type.hpp"
#include "../ast/call_expression.hpp"
#include "../ast/expression_result.hpp"
#include "../ast/llvm_value_wrapper_expression.hpp"

namespace Kepler::Type {

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
