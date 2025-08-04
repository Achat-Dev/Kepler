#pragma once

#include "ast/expression_result.hpp"

#include <memory>

namespace Kepler::AST {

    class Expression {
    public:
        virtual ~Expression() = default;
        virtual std::unique_ptr<ExpressionResult> codegen() = 0;
    };

}
