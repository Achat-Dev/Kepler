#include <cctype>
#include <cstdio>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "ast/binary_expression.hpp"
#include "ast/call_expression.hpp"
#include "ast/expression.hpp"
#include "ast/for_expression.hpp"
#include "ast/function.hpp"
#include "ast/if_expression.hpp"
#include "ast/number_expression.hpp"
#include "ast/parameter_data.hpp"
#include "ast/prototype.hpp"
#include "ast/return_expression.hpp"
#include "ast/variable_expression.hpp"
#include "ast/variable_definition_expression.hpp"
#include "lexer.hpp"
#include "log.hpp"
#include "type.hpp"
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
    static std::unique_ptr<AST::Prototype> parse_prototype(TypeToken type, std::string identifier);
    static std::unique_ptr<AST::Function> parse_function(TypeToken type, std::string identifier);
    static std::unique_ptr<AST::Prototype> parse_extern();
    static std::unique_ptr<AST::Function> parse_top_level_expression();
    static std::unique_ptr<AST::Expression> parse_if();
    static std::unique_ptr<AST::Expression> parse_for();
    static std::optional<std::vector<std::unique_ptr<AST::Expression>>> parse_for_body();
    static std::unique_ptr<AST::Expression> parse_return();
    static std::unique_ptr<AST::Expression> parse_local_variable();
    static bool handle_function(TypeToken type, std::string identifier);

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
            case Lexer::Token_DataType: return parse_local_variable();
            case '(': return parse_parenthesis();
            case '-': return parse_negative();
            default:
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": unknown token when expected expression. Current token: ", current_token);
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

    static std::unique_ptr<AST::Prototype> parse_prototype(TypeToken type, std::string identifier) {
        if (identifier.empty()) {
            read_next_token(); // eat data type
            if (current_token != Lexer::Token_Identifier) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected function name in prototype");
                return nullptr;
            }

            identifier = Lexer::get_identifier();
            read_next_token(); // eat identifier
            if (current_token != '(') {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '(' after function name in prototype");
                return nullptr;
            }
        }

        std::vector<AST::ParameterData> args;
        read_next_token(); // eat '('

        if (current_token != Lexer::Token_DataType && current_token != ')') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected data type or ')' after '(' in prototype");
            return nullptr;
        }

        while (current_token == Lexer::Token_DataType) {
            TypeToken type = Lexer::get_type();

            read_next_token(); // eat data type
            if (current_token != Lexer::Token_Identifier) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after data type in prototype");
                return nullptr;
            }

            args.push_back({ type, std::move(Lexer::get_identifier()) });

            read_next_token(); // eat identifier
            if (current_token == ',') {
                read_next_token(); // eat ','

                if (current_token != Lexer::Token_DataType) {
                    log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected data type after ',' in prototype");
                    return nullptr;
                }
            }
        }

        if (current_token != ')') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ')' after function parameters in prototype");
            return nullptr;
        }

        read_next_token(); // eat ')'
        return std::make_unique<AST::Prototype>(type, std::move(identifier), std::move(args));
    }

    static std::unique_ptr<AST::Function> parse_function(TypeToken type, std::string identifier) {
        //read_next_token(); // eat 'function' keyword
        auto prototype = parse_prototype(type, std::move(identifier));
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

        if (current_token != Lexer::Token_DataType) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected data type after 'extern'");
            return nullptr;
        }

        return parse_prototype(Lexer::get_type(), "");
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
        if (current_token != '(') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '(' after 'for'");
            return nullptr;
        }

        read_next_token(); // eat '('
        if (current_token != Lexer::Token_DataType) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected data type after '(' in 'for'");
            return nullptr;
        }

        TypeToken type = Lexer::get_type();
        read_next_token(); // eat data type
        if (current_token != Lexer::Token_Identifier) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after data type in 'for'");
            return nullptr;
        }

        std::string variable_name = Lexer::get_identifier();
        read_next_token(); // eat identifier
        if (current_token != ':') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ':' after identifier in 'for'");
            return nullptr;
        }
        read_next_token(); // eat ':'

        std::unique_ptr<AST::Expression> start = parse_expression();
        if (!start) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid start expression in 'for'");
            return nullptr;
        }

        // Only end value is given, start and step are implicit
        if (current_token == ')') {
            read_next_token(); // eat ')'
            std::optional<std::vector<std::unique_ptr<AST::Expression>>> body = parse_for_body();
            if (!body) {
                return nullptr;
            }

            std::unique_ptr<AST::BinaryExpression> assignment = std::make_unique<AST::BinaryExpression>('=', std::make_unique<AST::VariableExpression>(variable_name), std::make_unique<AST::NumberExpression>(0));
            std::unique_ptr<AST::VariableDefinitionExpression> variable = std::make_unique<AST::VariableDefinitionExpression>(type, variable_name, std::move(assignment));

            // 'start' is the end value
            return std::make_unique<AST::ForExpression>(std::move(variable), std::move(start), nullptr, std::move(*body));
        }

        if (current_token != ',') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ',' or ')' after first expression in 'for'");
            return nullptr;
        }
        read_next_token(); // eat ','

        std::unique_ptr<AST::Expression> end = parse_expression();
        if (!end) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected end expression after ','");
            return nullptr;
        }

        // Only start and end value are given, step is implicit
        if (current_token == ')') {
            read_next_token(); // eat ')'

            std::optional<std::vector<std::unique_ptr<AST::Expression>>> body = parse_for_body();
            if (!body) {
                return nullptr;
            }

            std::unique_ptr<AST::BinaryExpression> assignment = std::make_unique<AST::BinaryExpression>('=', std::make_unique<AST::VariableExpression>(variable_name), std::move(start));
            std::unique_ptr<AST::VariableDefinitionExpression> variable = std::make_unique<AST::VariableDefinitionExpression>(type, variable_name, std::move(assignment));

            return std::make_unique<AST::ForExpression>(std::move(variable), std::move(end), nullptr, std::move(*body));
        }

        if (current_token != ',') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected ',' or ')' after second expression in 'for'");
            return nullptr;
        }
        read_next_token(); // eat ','

        std::unique_ptr<AST::Expression> step = parse_expression();
        if (!step) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected step expression after ','");
            return nullptr;
        }

        if (current_token != ')') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": after ')' after step expression in 'for'");
            return nullptr;
        }
        read_next_token(); // eat ')'

        std::optional<std::vector<std::unique_ptr<AST::Expression>>> body = parse_for_body();
        if (!body) {
            return nullptr;
        }

        std::unique_ptr<AST::BinaryExpression> assignment = std::make_unique<AST::BinaryExpression>('=', std::make_unique<AST::VariableExpression>(variable_name), std::move(start));
        std::unique_ptr<AST::VariableDefinitionExpression> variable = std::make_unique<AST::VariableDefinitionExpression>(type, variable_name, std::move(assignment));

        return std::make_unique<AST::ForExpression>(std::move(variable), std::move(end), std::move(step), std::move(*body));
    }

    static std::optional<std::vector<std::unique_ptr<AST::Expression>>> parse_for_body() {
        std::vector<std::unique_ptr<AST::Expression>> body;
        while (current_token != Lexer::Token_End) {
            std::unique_ptr<AST::Expression> expression = parse_expression();
            if (!expression) {
                log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in 'for' body");
                return std::nullopt;
            }
            body.push_back(std::move(expression));
        }

        if (body.size() == 0) {
            log(LogStyle::WARNING, "[ Parsing warning ]", LogStyle::DEFAULT, ": empty loop body detected");
        }

        read_next_token(); // eat 'end'
        return std::move(body);
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

    // TODO: implement casting ('(' after data type)
    static std::unique_ptr<AST::Expression> parse_local_variable() {
        TypeToken type = Lexer::get_type();

        read_next_token(); // eat data type
        if (current_token != Lexer::Token_Identifier) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after data type in local variable definition");
            return nullptr;
        }
        std::string variable_name = Lexer::get_identifier();

        read_next_token(); // eat idetifier
        if (current_token != '=') {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected '=' after data type identifier in local variable definition");
            return nullptr;
        }
        read_next_token(); // eat '='

        std::unique_ptr<AST::Expression> value = parse_expression();
        if (!value) {
            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": invalid expression in variable assignment in local variable definition");
            return nullptr;
        }

        std::unique_ptr<AST::BinaryExpression> assignment_expression = std::make_unique<AST::BinaryExpression>('=', std::make_unique<AST::VariableExpression>(variable_name), std::move(value));

        return std::make_unique<AST::VariableDefinitionExpression>(type, variable_name, std::move(assignment_expression));
    }

    static bool handle_function(TypeToken type, std::string identifier) {
        if (auto ast = parse_function(type, std::move(identifier))) {
            if (auto ir = ast->codegen()) {
                log("> parsed a function definition <");
                ir->print(llvm::errs());
                return true;
            }
        }
        return false;
    }

    int get_current_token() {
        return current_token;
    }

    int read_next_token() {
        return current_token = Lexer::read_token();
    }

    bool handle_data_type() {
        TypeToken type = Lexer::get_type();
        read_next_token(); // eat data type

        // Variable definition or function definition
        if (current_token == Lexer::Token_Identifier) {
            std::string identifier = Lexer::get_identifier();
            read_next_token(); // eat identifier

            // Variable definition
            if (current_token == '=') {
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": global variables are not supported yet");
                return false;
            }
            // Function definition
            else if (current_token == '(') {
                return handle_function(type, std::move(identifier));
            }
        }

        log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": expected identifier after data type on top level");
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

}
