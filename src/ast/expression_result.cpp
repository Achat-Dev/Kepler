#include <llvm/IR/Value.h>
#include <memory>

#include "expression_result.hpp"

namespace Kepler::AST {

    const bool ExpressionResult::is_valid() const {
        return flags & ExpressionResultFlags::Valid;
    }

    const bool ExpressionResult::is_return_statement() const {
        return flags & ExpressionResultFlags::Return;
    }

    const bool ExpressionResult::is_returnable() const {
        return flags & ExpressionResultFlags::Returnable;
    }

    const bool ExpressionResult::is_assignable() const {
        return flags & ExpressionResultFlags::Returnable;
    }

    const bool ExpressionResult::forms_qualified_return() const {
        return flags & ExpressionResultFlags::QualifiedReturn;
    }

    llvm::Value* ExpressionResult::get_value() const {
        return value;
    }

    void ExpressionResult::set_value(llvm::Value* new_value) {
        value = new_value;
    }

    const unsigned int ExpressionResult::get_flags() const {
        return flags;
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create(llvm::Value* value, unsigned int flags) {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(value, flags));
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_invalid() {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(nullptr, 0));
    }

}
