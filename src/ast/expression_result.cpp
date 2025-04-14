#include <llvm/IR/Value.h>
#include <memory>

#include "expression_result.hpp"

namespace Kepler::AST {

    bool ExpressionResult::is_valid() const {
        return status != ExpressionStatus::Invalid;
    }

    bool ExpressionResult::is_return_statement() const {
        return is_return;
    }

    llvm::Value* ExpressionResult::get_value() const {
        return value;
    }

    void ExpressionResult::set_value(llvm::Value* new_value) {
        value = new_value;
    }

    const ExpressionStatus ExpressionResult::get_status() const {
        return status;
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_valid(llvm::Value* value) {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(value, ExpressionStatus::Valid, false));
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_returning(llvm::Value* value) {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(value, ExpressionStatus::Valid, true));
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_not_returnable() {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(nullptr, ExpressionStatus::NotReturnable, false));
    }

    std::unique_ptr<ExpressionResult> ExpressionResult::create_invalid() {
        return std::unique_ptr<ExpressionResult>(new ExpressionResult(nullptr, ExpressionStatus::Invalid, false));
    }

}
