#pragma once

#include <llvm/IR/Value.h>
#include <memory>

#include "ast.hpp"

namespace Kepler {

    const std::unique_ptr<AST::Expression> log_error(const std::string& message);
    const std::unique_ptr<AST::Prototype> log_errorp(const std::string& message);
    llvm::Value* log_errorv(const std::string& message);

}
