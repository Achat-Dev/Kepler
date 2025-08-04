#pragma once

#include "ast/expression.hpp"
#include "ast/expression_result.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    class LLVMValueWrapperExpression: public Expression {
    private:
        llvm::Value* value;
        const Type::TypeToken type;
        const unsigned int flags;

    public:
        LLVMValueWrapperExpression(llvm::Value* value, Type::TypeToken type, unsigned int flags)
            : value(value), type(type), flags(flags) {}
        std::unique_ptr<ExpressionResult> codegen() override;
    };

}
