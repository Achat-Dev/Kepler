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

    class NumberExpression: public Expression {
    private:
        double value;

    public:
        NumberExpression(double value) : value(value) {}
        llvm::Value* codegen() override;
    };

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
