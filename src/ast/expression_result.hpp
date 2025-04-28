#pragma once

#include <llvm/IR/Value.h>
#include <memory>

namespace Kepler::AST {

    enum ExpressionResultFlags {
        Valid = 0b0001,
        Returnable = 0b0010,
        Return = 0b0100,
        QualifiedReturn = 0b1000
    };

    class ExpressionResult {
    private:
        llvm::Value* value;
        const unsigned int flags;

        ExpressionResult(llvm::Value* value, unsigned int flags)
            : value(value), flags(flags) {}

    public:
        const bool is_valid() const;
        const bool is_return_statement() const;
        const bool is_returnable() const;
        const bool is_assignable() const;
        const bool forms_qualified_return() const;
        llvm::Value* get_value() const;
        void set_value(llvm::Value* new_value);
        const unsigned int get_flags() const;

        static std::unique_ptr<ExpressionResult> create(llvm::Value* value, unsigned int flags);
        static std::unique_ptr<ExpressionResult> create_invalid();
    };

}
