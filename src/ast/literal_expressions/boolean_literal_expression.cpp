#include "ast/literal_expressions/boolean_literal_expression.hpp"

#include "ast/expression_result.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/target_type_stack.hpp"
#include "types/type_token.hpp"

#include <llvm/IR/Constants.h>
#include <memory>

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> BooleanLiteralExpression::codegen() {
        const Type::TypeToken type = Type::TargetTypeStack::top();
        if (type != Type::TypeToken::Bool && type != Type::TypeToken::None && type != Type::TypeToken::TMap) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a boolean literal");
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(llvm::ConstantInt::getBool(Compiler::get_context(), value), Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
