#include <llvm/IR/Instructions.h>
#include <optional>
#include <string>

#include "../compiler.hpp"
#include "../log.hpp"
#include "../variables/local_variables.hpp"
#include "../variables/variable_data.hpp"
#include "expression_result.hpp"
#include "variable_expression.hpp"

namespace Kepler::AST {

    std::unique_ptr<ExpressionResult> VariableExpression::codegen() {
        std::optional<LocalVariables::VariableData> variable_data = LocalVariables::get(name);
        if (!variable_data) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": unknown variable name '", name, '\'');
            return ExpressionResult::create_invalid();
        }
        return ExpressionResult::create(Compiler::get_builder().CreateLoad(Type::get_by_token(variable_data->type), variable_data->variable, name.c_str()), variable_data->type, ExpressionResultFlags::Valid | ExpressionResultFlags::Returnable);
    }

    const std::string& VariableExpression::get_name() const {
        return name;
    }

}
