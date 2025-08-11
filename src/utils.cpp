#include "utils.hpp"

#include "ast/call_expression.hpp"
#include "ast/expression_result.hpp"
#include "ast/literal_expressions/string_literal_expression.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"

#include <cassert>
#include <exception>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Kepler {

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* f, Type::TypeToken type, llvm::StringRef variable_name) {
        llvm::Type* llvm_type = Type::get_by_token(type);
        llvm::IRBuilder<> tmp_builder(&f->getEntryBlock(), f->getEntryBlock().begin());
        llvm::AllocaInst* alloca = tmp_builder.CreateAlloca(llvm_type, nullptr, variable_name);

        if (type == Type::TypeToken::TMap) {
            llvm::Constant* default_tmap = llvm::ConstantAggregateZero::get(llvm_type);
            Compiler::get_builder().CreateStore(default_tmap, alloca);
        }

        return alloca;
    }

    void throw_runtime_error(const std::string &message) {
        std::stringstream error_message;
        error_message << LogStyle::ERROR << "[ Runtime error ]" << LogStyle::DEFAULT << ": " << message;

        std::unique_ptr<AST::StringLiteralExpression> arg_expression = std::make_unique<AST::StringLiteralExpression>(std::move(error_message.str()));
        std::vector<std::unique_ptr<AST::Expression>> args;
        args.push_back(std::move(arg_expression));

        AST::CallExpression call_expression = AST::CallExpression("error", std::move(args));
        std::unique_ptr<AST::ExpressionResult> call_er = call_expression.codegen();

        assert(call_er->is_valid() && "[ Assertion ]: failed to throw runtime error, the expression result is invalid");
    }

    void emergency_exit(const std::string& message) {
        log(LogStyle::UNSUPPORTED, "[ Emergency exit ]", LogStyle::DEFAULT, ": ", message);
        log(LogStyle::UNSUPPORTED,
            "Roses are red, violets are blue,\n",
            "I reached some code that I never should do.\n",
            "Now here I am, with no helping hand,\n",
            "And a crash and stack trace I don’t understand.");
        std::terminate();
    }

}
