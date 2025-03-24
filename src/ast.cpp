#include <cmath>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ADT/APFloat.h>
#include <map>
#include <memory>
#include <vector>

#include "ast.hpp"
#include "log.hpp"

namespace Kepler::AST {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;
    static std::map<std::string, llvm::Value*> named_values;

    llvm::Value* NumberExpression::codegen() {
        return llvm::ConstantFP::get(*context, llvm::APFloat(value));
    }

    llvm::Value* VariableExpression::codegen() {
        llvm::Value* v = named_values[name];
        if (!v) {
            log_errorv("unknown variable name");
        }
        return v;
    }

    llvm::Value* BinaryExpression::codegen() {
        llvm::Value* l = lhs->codegen();
        llvm::Value* r = rhs->codegen();

        if (!l || !r) {
            return nullptr;
        }

        switch (op) {
            case '<':
                l = builder->CreateFCmpULT(l, r, "cmptmp");
                return builder->CreateUIToFP(l, llvm::Type::getDoubleTy(*context), "booltmp");
            case '>':
                l = builder->CreateFCmpUGT(l, r, "cmptmp");
                return builder->CreateUIToFP(l, llvm::Type::getDoubleTy(*context), "booltmp");
            case '+': return builder->CreateFAdd(l, r, "addtmp");
            case '-': return builder->CreateFSub(l, r, "subtmp");
            case '*': return builder->CreateFMul(l, r, "multmp");
            case '/': return builder->CreateFDiv(l, r, "divtmp");
            default: return log_errorv("invalid binary operator");
        }
    }

    llvm::Value* CallExpression::codegen() {
        llvm::Function* calleef = module->getFunction(callee);
        if (!calleef) {
            return log_errorv("unknown function called");
        }

        if (calleef->arg_size() != args.size()) {
            return log_errorv("incorrect number of arguments passed to function");
        }

        std::vector<llvm::Value*> argsv;
        for (unsigned i = 0, e = args.size(); i != e; i++) {
            argsv.push_back(args[i]->codegen());
            if (!argsv.back()) {
                return nullptr;
            }
        }

        return builder->CreateCall(calleef, argsv, "calltmp");
    }

    llvm::Function* Prototype::codegen() {
        std::vector<llvm::Type*> types(args.size(), llvm::Type::getDoubleTy(*context));
        llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getDoubleTy(*context), types, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module.get());

        unsigned idx = 0;
        for (auto& arg : f->args()) {
            arg.setName(args[idx++]);
        }

        return f;
    }

    /*
    * This method currently has a bug:
    * If the FunctionAST::codegen() method finds an existing IR Function, it does not validate its signature against the definition’s own prototype. This means that an earlier ‘extern’ declaration will take precedence over the function definition’s signature, which can cause codegen to fail, for instance if the function arguments are named differently
    */
    llvm::Function* Function::codegen() {
        llvm::Function* f = module->getFunction(prototype->get_name());
        if (!f) {
            f = prototype->codegen();
        }

        if (!f) {
            return nullptr;
        }

        if (!f->empty()) {
            return static_cast<llvm::Function*>(log_errorv("function cannot be redefined"));
        }

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context, "entry", f);
        builder->SetInsertPoint(bb);

        // record function arguments
        named_values.clear();
        for (auto& arg : f->args()) {
            named_values[std::string(arg.getName())] = &arg;
        }

        if (llvm::Value* return_value = body->codegen()) {
            builder->CreateRet(return_value);
            llvm::verifyFunction(*f);
            return f;
        }

        f->eraseFromParent();
        return nullptr;
    }

}
