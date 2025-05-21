#include <llvm/IR/Constants.h>
#include <memory>

#include "boolean_value_expression.hpp"
#include "../expression_result.hpp"
#include "../../compiler.hpp"
#include "../../log.hpp"
#include "../../types/target_type_stack.hpp"
#include "../../types/type.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> BooleanValueExpression::codegen() {
        const Type::TypeToken type = Type::TargetTypeStack::top();
        if (type != Type::TypeToken::Bool && type != Type::TypeToken::None) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type mismatch: can't create a value of type '", type, "' from a boolean value");
            return ExpressionResult::create_invalid();
        }

        return ExpressionResult::create(llvm::ConstantInt::getBool(Compiler::get_context(), value), Type::TypeToken::Bool, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

}
