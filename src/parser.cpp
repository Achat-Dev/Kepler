#include <cctype>
#include <cstdio>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "ast/binary_expression.hpp"
#include "ast/call_expression.hpp"
#include "ast/expression.hpp"
#include "ast/for_expression.hpp"
#include "ast/function.hpp"
#include "ast/if_expression.hpp"
#include "ast/number_expression.hpp"
#include "ast/prototype.hpp"
#include "ast/return_expression.hpp"
#include "ast/variable_expression.hpp"
#include "ast/variable_definition_expression.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "parser.hpp"

namespace Kepler::Parser {

    static int get_token_precedence();

    static std::unique_ptr<AST::Expression> parse_number();
    static std::unique_ptr<AST::Expression> parse_parenthesis();
    static std::unique_ptr<AST::Expression> parse_negative();
    static std::unique_ptr<AST::Expression> parse_expression();
    static std::unique_ptr<AST::Expression> parse_identifier();
    static std::unique_ptr<AST::Expression> parse_primary();
    static std::unique_ptr<AST::Expression> parse_binop_rhs(int expression_precedence, std::unique_ptr<AST::Expression> lhs);
    static std::unique_ptr<AST::Prototype> parse_prototype();
    static std::unique_ptr<AST::Function> parse_function();
    static std::unique_ptr<AST::Prototype> parse_extern();
    static std::unique_ptr<AST::Function> parse_top_level_expression();
    static std::unique_ptr<AST::Expression> parse_if();
    static std::unique_ptr<AST::Expression> parse_for();
    static std::unique_ptr<AST::Expression> parse_return();
    static std::unique_ptr<AST::Expression> parse_local_variable();

    static int current_token;

    // Higher values mean higher precedence
    static std::map<char, int> binop_precedence_map = {
        { '=', 2 },
        { '<', 10 },
        { '>', 10 },
        { '+', 20 },
        { '-', 20 },
        { '*', 40 },
        { '/', 40 }
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
        read_next_token(); // eat the number
        return std::move(result);
    }

    static std::unique_ptr<AST::Expression> parse_parenthesis() {
        read_next_token(); // eat '('

        auto expression = parse_expression();
        if (!expression) {
            return nullptr;
        }

        if (current_token != ')') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ')'");
            return nullptr;
        }

        read_next_token(); // eat ')'
        return expression;
    }

    static std::unique_ptr<AST::Expression> parse_negative() {
        auto lhs = std::make_unique<AST::NumberExpression>(0);
        return parse_binop_rhs(0, std::move(lhs));
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
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ')' or ',' in function argument list");
                    return nullptr;
                }

                read_next_token(); // eat ','
            }
        }

        read_next_token(); // eat ')'

        return std::make_unique<AST::CallExpression>(identifier, std::move(args));
    }

    static std::unique_ptr<AST::Expression> parse_primary() {
        switch (current_token) {
            case Lexer::Token_Identifier: return parse_identifier();
            case Lexer::Token_Number: return parse_number();
            // Case fallthrough for if and parse_elseif
            case Lexer::Token_If:
                case Lexer::Token_Parsing_Elseif:
                    return parse_if();
            case Lexer::Token_For: return parse_for();
            case Lexer::Token_Return: return parse_return();
            case Lexer::Token_Var: return parse_local_variable();
            case '(': return parse_parenthesis();
            case '-': return parse_negative();
            default:
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": unknown token when expected expression. Current token:", current_token);
                return nullptr;
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
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": no right hand side value when parsing binary operator '", (char)binop, "'");
                return nullptr;
            }

            int next_precedence = get_token_precedence();
            if (token_precedence < next_precedence) {
                rhs = parse_binop_rhs(token_precedence + 1, std::move(rhs));
                if (!rhs) {
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": no right hand side value when parsing binary operator '", (char)binop, "'");
                    return nullptr;
                }
            }

            lhs = std::make_unique<AST::BinaryExpression>(binop, std::move(lhs), std::move(rhs));
        }
    }

    static std::unique_ptr<AST::Prototype> parse_prototype() {
        if (current_token != Lexer::Token_Identifier) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected function name in prototype");
            return nullptr;
        }

        std::string function_name = Lexer::get_identifier();
        read_next_token();
        if (current_token != '(') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '(' after function name in prototype");
            return nullptr;
        }

        std::vector<std::string> arg_names;
        read_next_token();
        while (current_token == Lexer::Token_Identifier) {
            arg_names.push_back(Lexer::get_identifier());

            read_next_token();
            if (current_token == ',') {
                read_next_token();
            }
        }
        if (current_token != ')') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ')' after function arguments in prototype");
            return nullptr;
        }

        read_next_token(); // eat ')'
        return std::make_unique<AST::Prototype>(function_name, std::move(arg_names));
    }

    static std::unique_ptr<AST::Function> parse_function() {
        read_next_token(); // eat 'function' keyword
        auto prototype = parse_prototype();
        if (!prototype) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid function prototype");
            return nullptr;
        }

        std::vector<std::unique_ptr<AST::Expression>> body;
        while (current_token != Lexer::Token_End) {
            auto expression = parse_expression();
            if (!expression) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in function body");
                return nullptr;
            }
            body.push_back(std::move(expression));
        }

        read_next_token(); // eat 'end'

        return std::make_unique<AST::Function>(std::move(prototype), std::move(body));
    }

    static std::unique_ptr<AST::Prototype> parse_extern() {
        read_next_token(); // eat 'extern' keyword
        return parse_prototype();
    }

    // top level expression are anonymous functions with zero arguments
    static std::unique_ptr<AST::Function> parse_top_level_expression() {
        if (auto expression = parse_expression()) {
            auto prototype = std::make_unique<AST::Prototype>();
            std::vector<std::unique_ptr<AST::Expression>> body;
            body.push_back(std::move(expression));
            return std::make_unique<AST::Function>(std::move(prototype), std::move(body));
        }
        return nullptr;
    }

    static std::unique_ptr<AST::Expression> parse_if() {
        bool is_elseif = current_token == Lexer::Token_Parsing_Elseif;

        read_next_token(); // eat 'if' or 'elseif'

        // Parse 'if' condition
        std::unique_ptr<AST::Expression> condition = parse_expression();
        if (!condition) {
            if (is_elseif) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected condition after 'elseif'");
            }
            else {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected condition after 'if'");
            }
            return nullptr;
        }

        // Parse 'if' body
        std::vector<std::unique_ptr<AST::Expression>> if_body;
        while (current_token != Lexer::Token_Elseif && current_token != Lexer::Token_Else && current_token != Lexer::Token_End) {
            auto expression = parse_expression();
            if (!expression) {
                if (is_elseif) {
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'elseif' body");
                }
                else {
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'if' body");
                }
                return nullptr;
            }
            if_body.push_back(std::move(expression));
        }

        if (if_body.size() == 0) {
            if (is_elseif) {
                log(LogStyle::WARNING, "[ Parsing warning ]", LogStyle::DEFAULT, ": empty 'elseif' body detected");
            }
            else {
             log(LogStyle::WARNING, "[ Parsing warning ]", LogStyle::DEFAULT, ": empty 'if' body detected");
            }
        }

        // Setup behaviour for different 'if' ending cases
        if (current_token == Lexer::Token_Else) {
            read_next_token(); // eat 'else'
        }
        else if (current_token == Lexer::Token_Elseif) {
            current_token = Lexer::Token_Parsing_Elseif;
        }
        else if (current_token == Lexer::Token_End) {
            if (!is_elseif) {
                read_next_token(); // eat 'end'
            }
            return std::make_unique<AST::IfExpression>(std::move(condition), std::move(if_body), std::vector<std::unique_ptr<AST::Expression>>());
        }

        // Parse 'else' body
        std::vector<std::unique_ptr<AST::Expression>> else_body;
        while (current_token != Lexer::Token_End) {
            auto expression = parse_expression();
            if (!expression) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'else' body");
                return nullptr;
            }
            else_body.push_back(std::move(expression));
        }

        if (else_body.size() == 0) {
            log(LogStyle::WARNING, "[ Parsing warning ]", LogStyle::DEFAULT, ": empty else body detected");
        }

        if (!is_elseif) {
            read_next_token(); // eat 'end'
        }

        return std::make_unique<AST::IfExpression>(std::move(condition), std::move(if_body), std::move(else_body));
    }

    static std::unique_ptr<AST::Expression> parse_for() {
        read_next_token(); // eat 'for'

        if (current_token != Lexer::Token_Identifier) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after 'for'");
            return nullptr;
        }

        std::string variable_name = Lexer::get_identifier();

        read_next_token();
        if (current_token != '=') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '=' after identifier in 'for'");
            return nullptr;
        }
        read_next_token(); // eat '='

        std::unique_ptr<AST::Expression> start = parse_expression();
        if (!start) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid start expression in 'for'");
            return nullptr;
        }
        if (current_token != ',') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ',' after 'for' variable initialisation");
            return nullptr;
        }
        read_next_token(); // eat ','

        std::unique_ptr<AST::Expression> end = parse_expression();
        if (!end) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected end value after ','");
            return nullptr;
        }

        std::unique_ptr<AST::Expression> step;
        if (current_token == ',') {
            read_next_token(); // eat ','
            step = parse_expression();
            if (!step) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected step value after ','");
                return nullptr;
            }
        }

        std::vector<std::unique_ptr<AST::Expression>> body;
        while (current_token != Lexer::Token_End) {
            auto expression = parse_expression();
            if (!expression) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'for' body");
                return nullptr;
            }
            body.push_back(std::move(expression));
        }

        if (body.size() == 0) {
            log(LogStyle::WARNING, "[ Parsing warning ]", LogStyle::DEFAULT, ": empty loop body detected");
        }

        read_next_token(); // eat 'end'

        return std::make_unique<AST::ForExpression>(variable_name, std::move(start), std::move(end), std::move(step), std::move(body));
    }

    static std::unique_ptr<AST::Expression> parse_return() {
        read_next_token(); // eat 'return' keyword
        auto expression = parse_expression();
        if (!expression) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected expression after 'return'");
            return nullptr;
        }

        return std::make_unique<AST::ReturnExpression>(std::move(expression));
    }

    static std::unique_ptr<AST::Expression> parse_local_variable() {
        read_next_token(); // eat 'var'

        if (current_token != Lexer::Token_Identifier) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after 'var'");
            return nullptr;
        }
        std::string variable_name = Lexer::get_identifier();

        read_next_token();
        if (current_token != '=') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '=' after 'var' identifier");
            return nullptr;
        }
        read_next_token(); // eat '='

        std::unique_ptr<AST::Expression> value = parse_expression();
        if (!value) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'var' assignment");
            return nullptr;
        }

        std::unique_ptr<AST::BinaryExpression> assignment_expression = std::make_unique<AST::BinaryExpression>('=', std::make_unique<AST::VariableExpression>(variable_name), std::move(value));

        return std::make_unique<AST::VariableDefinitionExpression>(variable_name, std::move(assignment_expression));
    }

    int get_current_token() {
        return current_token;
    }

    int read_next_token() {
        return current_token = Lexer::read_token();
    }

    bool handle_function() {
        if (auto ast = parse_function()) {
            if (auto ir = ast->codegen()) {
                log("> parsed a function definition <");
                ir->print(llvm::errs());
                return true;
            }
        }
        return false;
    }

    bool handle_extern() {
        if (auto ast = parse_extern()) {
            if (auto ir = ast->codegen()) {
                log("> parsed an extern <");
                ir->print(llvm::errs());
                return true;
            }
        }
        return false;
    }

    bool handle_top_level_expression() {
        if (auto ast = parse_top_level_expression()) {
            if (auto ir = ast->codegen()) {
                log("> parsed a top level expression <");
                ir->print(llvm::errs());
                return true;
            }
        }
        return false;
    }

}
