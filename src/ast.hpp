#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>

namespace Kepler::AST {

    const bool initialise();
    const int write_file(const char* filename);

    class Expression {
    public:
        virtual ~Expression() = default;
        virtual llvm::Value* codegen() = 0;
    };

    // Data types
    class NumberExpression: public Expression {
    private:
        double value;

    public:
        NumberExpression(double value) : value(value) {}
        llvm::Value* codegen() override;
    };

    // Control flow
    class IfExpression: public Expression {
    private:
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Expression> if_branch;
        std::unique_ptr<Expression> else_branch;

    public:
        IfExpression(std::unique_ptr<Expression> condition,
            std::unique_ptr<Expression> if_branch,
            std::unique_ptr<Expression> else_branch)
            : condition(std::move(condition)),
              if_branch(std::move(if_branch)),
              else_branch(std::move(else_branch)) {}
        llvm::Value* codegen() override;
    };

    class ForExpression: public Expression {
    private:
        std::string variable_name;
        std::unique_ptr<Expression> start, end, step, body;

    public:
        ForExpression(std::string variable_name,
            std::unique_ptr<Expression> start,
            std::unique_ptr<Expression> end,
            std::unique_ptr<Expression> step,
            std::unique_ptr<Expression> body)
            : variable_name(variable_name),
              start(std::move(start)),
              end(std::move(end)),
              step(std::move(step)),
              body(std::move(body)) {}
        llvm::Value* codegen() override;
    };

    // Other
    class VariableExpression: public Expression {
    private:
        std::string name;

    public:
        VariableExpression(const std::string& name) : name(name) {}
        llvm::Value* codegen() override;
    };

    class BinaryExpression : public Expression {
    private:
        char op;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

    public:
        BinaryExpression(char op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        llvm::Value* codegen() override;
    };

    class CallExpression : public Expression {
    private:
        std::string callee;
        std::vector<std::unique_ptr<Expression>> args;

    public:
        CallExpression(const std::string& callee, std::vector<std::unique_ptr<Expression>> args)
            : callee(callee), args(std::move(args)) {}
        llvm::Value* codegen() override;
    };

    // Function
    class Prototype {
    private:
        std::string name;
        std::vector<std::string> args;

    public:
        Prototype() : name(""), args(std::vector<std::string>()) {}
        Prototype(const std::string& name, std::vector<std::string> args)
            : name(name), args(std::move(args)) {}
        llvm::Function* codegen();
        const std::string& get_name() const { return name; }
    };

    class Function {
    private:
        std::unique_ptr<Prototype> prototype;
        std::unique_ptr<Expression> body;

    public:
        Function(std::unique_ptr<Prototype> prototype, std::unique_ptr<Expression> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
        llvm::Function* codegen();
    };

}
