#include <llvm/IR/Value.h>

#include "../compiler.hpp"
#include "../log.hpp"
#include "variable_expression.hpp"

namespace Kepler::AST {

    llvm::Value* VariableExpression::codegen() {
        llvm::Value* v = Compiler::Internal::get_named_values()[name];
        if (!v) {
            log_errorv("unknown variable name");
        }
        return v;
    }

}
