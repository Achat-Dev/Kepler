#include <cctype>
#include <cstdio>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"

namespace Kepler::Parser {


    static std::unique_ptr<AST::Expression> log_error(const std::string& str);
    static std::unique_ptr<AST::Prototype> log_errorp(const std::string& str);

    static int get_token_precedence();

    static std::unique_ptr<AST::Expression> parse_number();
    static std::unique_ptr<AST::Expression> parse_parenthesis();
    static std::unique_ptr<AST::Expression> parse_expression();
    static std::unique_ptr<AST::Expression> parse_identifier();
    static std::unique_ptr<AST::Expression> parse_primary();
    static std::unique_ptr<AST::Expression> parse_binop_rhs(int expression_precedence, std::unique_ptr<AST::Expression> lhs);
    static std::unique_ptr<AST::Prototype> parse_prototype();
    static std::unique_ptr<AST::Function> parse_function();
    static std::unique_ptr<AST::Prototype> parse_extern();
    static std::unique_ptr<AST::Function> parse_top_level_expression();

    static std::unique_ptr<AST::Expression> log_error(const std::string& str) {
        std::fprintf(stderr, "Error: %s\n", str.c_str());
        return nullptr;
    }

    static std::unique_ptr<AST::Prototype> log_errorp(const std::string& str) {
        log_error(str);
        return nullptr;
    }

    static int current_token;

    // Higher values mean higher precedence
    static std::map<char, int> binop_precedence_map = {
        { '>', 10 },
        { '<', 10 },
        { '+', 20 },
        { '-', 20 },
        { '*', 40 },
        { '/', 40 },
        { '%', 40 }
    };
    static int get_token_precedence() {
        if (!isascii(current_token)) {
            return -1;
        }

        int precedence = binop_precedence_map[current_token];
        if (precedence <= 0) {
            return -1;
        }
        return precedence;
    }

    static std::unique_ptr<AST::Expression> parse_number() {
        auto result = std::make_unique<AST::NumberExpression>(Lexer::get_number_value());
        read_next_token();
        return std::move(result);
    }

    static std::unique_ptr<AST::Expression> parse_parenthesis() {
        read_next_token(); // eat '('

        auto expression = parse_expression();
        if (!expression) {
            return nullptr;
        }

        if (current_token != ')') {
            return log_error("expected ')'");
        }

        read_next_token(); // eat ')'
        return expression;
    }

    static std::unique_ptr<AST::Expression> parse_expression() {
        auto lhs = parse_primary();
        if (!lhs) {
            return nullptr;
        }

        return parse_binop_rhs(0, std::move(lhs));
    }

    static std::unique_ptr<AST::Expression> parse_identifier() {
        std::string identifier = Lexer::get_identifier();

        read_next_token(); // eat the current identifier

        if (current_token != '(') { // this means we are using a variable (no parethesis after identifier)
            return std::make_unique<AST::VariableExpression>(identifier);
        }

        read_next_token(); // eat '('
        std::vector<std::unique_ptr<AST::Expression>> args;
        if (current_token != ')') {
            while (true) {
                if (auto arg = parse_expression()) {
                    args.push_back(std::move(arg));
                }
                else {
                    return  nullptr;
                }

                if (current_token == ')') {
                    break;
                }

                if (current_token != ',') {
                    return log_error("expected ')' or ',' in function argument list");
                }

                read_next_token();
            }
        }

        read_next_token(); // eat ')'

        return std::make_unique<AST::CallExpression>(identifier, std::move(args));
    }

    static std::unique_ptr<AST::Expression> parse_primary() {
        switch (current_token) {
            case Lexer::Token_Identifier: return parse_identifier();
            case Lexer::Token_Number: return parse_number();
            case '(': return parse_parenthesis();
            default: return log_error("unknown token when expected expression");
        }
    }

    static std::unique_ptr<AST::Expression> parse_binop_rhs(int expression_precedence, std::unique_ptr<AST::Expression> lhs) {
        while (true) {
            int token_precedence = get_token_precedence();
            if (token_precedence < expression_precedence) {
                return lhs;
            }

            int binop = current_token;
            read_next_token(); // eat binary operator

            auto rhs = parse_primary();
            if (!rhs) {
                return nullptr;
            }

            int next_precedence = get_token_precedence();
            if (token_precedence < next_precedence) {
                rhs = parse_binop_rhs(token_precedence + 1, std::move(rhs));
                if (!rhs) {
                    return nullptr;
                }
            }

            lhs = std::make_unique<AST::BinaryExpression>(binop, std::move(lhs), std::move(rhs));
        }
    }

    static std::unique_ptr<AST::Prototype> parse_prototype() {
        if (current_token != Lexer::Token_Identifier) {
            return log_errorp("expected function name in prototype");
        }

        std::string function_name = Lexer::get_identifier();
        read_next_token();
        if (current_token != '(') {
            return log_errorp("expected '(' after function name in prototype");
        }

        std::vector<std::string> arg_names;
        while (read_next_token() == Lexer::Token_Identifier) {
            arg_names.push_back(Lexer::get_identifier());
        }
        if (current_token != ')') {
            log_errorp("expected ')' after function arguments in prototype");
        }

        read_next_token(); // eat ')'
        return std::make_unique<AST::Prototype>(function_name, std::move(arg_names));
    }

    static std::unique_ptr<AST::Function> parse_function() {
        read_next_token();
        auto prototype = parse_prototype();
        if (!prototype) {
            return nullptr;
        }

        if (auto expression = parse_expression()) {
            return std::make_unique<AST::Function>(std::move(prototype), std::move(expression));
        }
        return nullptr;
    }

    static std::unique_ptr<AST::Prototype> parse_extern() {
        read_next_token(); // eat 'extern' keyword
        return parse_prototype();
    }

    // top level expression are anonymous functions with zero arguments
    static std::unique_ptr<AST::Function> parse_top_level_expression() {
        if (auto expression = parse_expression()) {
            auto prototype = std::make_unique<AST::Prototype>();
            return std::make_unique<AST::Function>(std::move(prototype), std::move(expression));
        }
        return nullptr;
    }

    const int get_current_token() {
        return current_token;
    }

    const int read_next_token() {
        return current_token = Lexer::read_token();
    }

    void handle_function() {
        if (parse_function()) {
            std::fprintf(stderr, "> parsed a function definition <\n");
        }
        else {
            read_next_token(); // skip token for error recovery
        }
    }

    void handle_extern() {
        if (parse_extern()) {
            std::fprintf(stderr, "> parsed an extern <\n");
        }
        else {
            read_next_token(); // skip token for error recovery
        }
    }

    void handle_top_level_expression() {
        if (parse_top_level_expression()) {
            std::fprintf(stderr, "> parsed a top level expression <\n");
        }
        else {
            read_next_token(); // skip token for error recovery
        }
    }

}
