#pragma once

#include <memory>

#include "expression_result.hpp"

namespace Kepler::AST {

    class Expression {
    public:
        virtual ~Expression() = default;
        virtual std::unique_ptr<ExpressionResult> codegen() = 0;
    };

}
