#include <cassert>

#include "operators.hpp"
#include "../ast/expression_result.hpp"
#include "../compiler.hpp"
#include "../log.hpp"
#include "../types/type.hpp"

namespace Kepler::Operators {

    std::unique_ptr<AST::ExpressionResult> create_add(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        assert((lhs->get_type() == rhs->get_type()) && "[ Assertion ]: 'create_add' expects ExpressionResults of the same type");

        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateAdd(lhs->get_value(), rhs->get_value(), "add");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFAdd(lhs->get_value(), rhs->get_value(), "add");
            return AST:: ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '+' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_sub(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateSub(lhs->get_value(), rhs->get_value(), "sub");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFSub(lhs->get_value(), rhs->get_value(), "sub");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '-' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_mul(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateMul(lhs->get_value(), rhs->get_value(), "mul");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFMul(lhs->get_value(), rhs->get_value(), "mul");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '*' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_div(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateSDiv(lhs->get_value(), rhs->get_value(), "div");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFDiv(lhs->get_value(), rhs->get_value(), "div");
            return AST::ExpressionResult::create(value, type, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '/' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_less_than(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSLT(lhs->get_value(), rhs->get_value(), "lt");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpULT(lhs->get_value(), rhs->get_value(), "lt");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '<' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_greater_than(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSGT(lhs->get_value(), rhs->get_value(), "gt");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpUGT(lhs->get_value(), rhs->get_value(), "gt");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '>' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpEQ(lhs->get_value(), rhs->get_value(), "eq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpUEQ(lhs->get_value(), rhs->get_value(), "eq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (type == Type::TypeToken::Bool) {
            llvm::Value* value = Compiler::get_builder().CreateICmpEQ(lhs->get_value(), rhs->get_value(), "eq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '==' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_not_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpNE(lhs->get_value(), rhs->get_value(), "neq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpUNE(lhs->get_value(), rhs->get_value(), "neq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (type == Type::TypeToken::Bool) {
            llvm::Value* value = Compiler::get_builder().CreateICmpNE(lhs->get_value(), rhs->get_value(), "neq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '!=' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_less_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSLE(lhs->get_value(), rhs->get_value(), "leq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpULE(lhs->get_value(), rhs->get_value(), "leq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '<=' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

    std::unique_ptr<AST::ExpressionResult> create_greater_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs) {
        const Type::TypeToken type = lhs->get_type();
        if (Type::is_integer_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateICmpSGE(lhs->get_value(), rhs->get_value(), "geq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }
        else if (Type::is_floating_point_type(type)) {
            llvm::Value* value = Compiler::get_builder().CreateFCmpUGE(lhs->get_value(), rhs->get_value(), "geq");
            return AST::ExpressionResult::create(value, Type::TypeToken::Bool, AST::ExpressionResultFlags::Valid | AST::ExpressionResultFlags::Returnable);
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '>=' operation between type '", type, "' and type '", rhs->get_type(), "' is not supported");
        return AST::ExpressionResult::create_invalid();
    }

}
