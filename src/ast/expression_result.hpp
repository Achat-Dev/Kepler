#pragma once

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    enum class ExpressionStatus {
        Return,
        Returnable,
        NotReturnable,
        Invalid,
    };

    class ExpressionResult {
    private:
        llvm::Value* value;
        const ExpressionStatus status;

        ExpressionResult(llvm::Value* value, ExpressionStatus status)
            : value(value), status(status) {}

    public:
        bool is_valid() const;
        bool is_return_statement() const;
        llvm::Value* get_value() const;
        void set_value(llvm::Value* new_value);
        const ExpressionStatus get_status() const;

        static std::unique_ptr<ExpressionResult> create_valid(llvm::Value* value);
        static std::unique_ptr<ExpressionResult> create_return(llvm::Value* value);
        static std::unique_ptr<ExpressionResult> create_not_returnable();
        static std::unique_ptr<ExpressionResult> create_invalid();
    };

}
