#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Kepler::AST {

    class Expression {
    public:
        virtual ~Expression() = default;
    };

    class NumberExpression: public Expression {
    private:
        double value;

    public:
        NumberExpression(double value) : value(value) {}
    };

    class VariableExpression: public Expression {
    private:
        std::string name;

    public:
        VariableExpression(const std::string& name) : name(name) {}
    };

    class BinaryExpression : public Expression {
    private:
        char op;
        std::unique_ptr<Expression> lhs;
        std::unique_ptr<Expression> rhs;

    public:
        BinaryExpression(char op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    };

    class CallExpression : public Expression {
    private:
        std::string callee;
        std::vector<std::unique_ptr<Expression>> args;

    public:
        CallExpression(const std::string& callee, std::vector<std::unique_ptr<Expression>> args)
            : callee(callee), args(std::move(args)) {}
    };

    class Prototype {
    private:
        std::string name;
        std::vector<std::string> args;

    public:
        Prototype() : name(""), args(std::vector<std::string>()) {}
        Prototype(const std::string& name, std::vector<std::string> args)
            : name(name), args(std::move(args)) {}
        const std::string& get_name() const { return name; }
    };

    class Function {
    private:
        std::unique_ptr<Prototype> prototype;
        std::unique_ptr<Expression> body;

    public:
        Function(std::unique_ptr<Prototype> prototype, std::unique_ptr<Expression> body)
            : prototype(std::move(prototype)), body(std::move(body)) {}
    };

}
