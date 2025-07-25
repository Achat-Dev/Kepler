#pragma once

#include <llvm/IR/Value.h>
#include <memory>

#include "../ast/expression_result.hpp"

namespace Kepler::Operators {

    std::unique_ptr<AST::ExpressionResult> create_add(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_sub(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_mul(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_div(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_less_than(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_greater_than(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_not_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_less_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);
    std::unique_ptr<AST::ExpressionResult> create_greater_equals(std::unique_ptr<AST::ExpressionResult> lhs, std::unique_ptr<AST::ExpressionResult> rhs);

}
