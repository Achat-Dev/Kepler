#include <cmath>
#include <cstdio>
#include <llvm/ADT/APFloat.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassInstrumentation.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "ast.hpp"
#include "log.hpp"

namespace Kepler::AST {

    static std::unique_ptr<llvm::LLVMContext> context;
    static std::unique_ptr<llvm::IRBuilder<>> builder;
    static std::unique_ptr<llvm::Module> module;

    static std::unique_ptr<llvm::FunctionPassManager> fpm;
    static std::unique_ptr<llvm::FunctionAnalysisManager> fam;
    static std::unique_ptr<llvm::LoopAnalysisManager> lam;
    static std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
    static std::unique_ptr<llvm::ModuleAnalysisManager> mam;
    static std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
    static std::unique_ptr<llvm::StandardInstrumentations> si;

    static llvm::TargetMachine* target_machine;

    static std::map<std::string, llvm::Value*> named_values;

    const bool initialise() {
        context = std::make_unique<llvm::LLVMContext>();
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
        module = std::make_unique<llvm::Module>("Kepler", *context);

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string target_triple_string = llvm::sys::getDefaultTargetTriple();
        llvm::Triple target_triple(target_triple_string);
        module->setTargetTriple(target_triple);

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple_string, error);

        if (!target) {
            llvm::errs() << error;
            return false;
        }

        std::string cpu = "generic";
        std::string features = "";
        llvm::TargetOptions target_options;
        target_machine = target->createTargetMachine(target_triple, cpu, features, target_options, llvm::Reloc::PIC_);

        module->setDataLayout(target_machine->createDataLayout());

        // optimisation
        fpm = std::make_unique<llvm::FunctionPassManager>();
        fam = std::make_unique<llvm::FunctionAnalysisManager>();
        lam = std::make_unique<llvm::LoopAnalysisManager>();
        cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
        mam = std::make_unique<llvm::ModuleAnalysisManager>();
        pic = std::make_unique<llvm::PassInstrumentationCallbacks>();
        si = std::make_unique<llvm::StandardInstrumentations>(*context, true);
        si->registerCallbacks(*pic, mam.get());

        fpm->addPass(llvm::InstCombinePass());
        fpm->addPass(llvm::ReassociatePass());
        fpm->addPass(llvm::GVNPass());
        fpm->addPass(llvm::SimplifyCFGPass());

        llvm::PassBuilder passbuilder;
        passbuilder.registerModuleAnalyses(*mam);
        passbuilder.registerFunctionAnalyses(*fam);
        passbuilder.crossRegisterProxies(*lam, *fam, *cgam, *mam);

        return true;
    }

    const int write_file(const char* filename) {
        std::error_code ec;
        llvm::raw_fd_ostream out(filename, ec, llvm::sys::fs::OF_None);

        if (ec) {
            llvm::errs() << "Error: Failed to open output file: " << ec.message();
            return 1;
        }

        llvm::legacy::PassManager pass_manager;
        if (target_machine->addPassesToEmitFile(pass_manager, out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
            llvm::errs() << "Error: Target machine can't emit file of type 'Object File'";
            return 1;
        }

        pass_manager.run(*module);
        out.flush();
        llvm::outs() << "Wrote " << filename << "\n";

        return 0;
    }

    llvm::Value* NumberExpression::codegen() {
        return llvm::ConstantFP::get(*context, llvm::APFloat(value));
    }

    llvm::Value* IfExpression::codegen() {
        llvm::Value* conditionv = condition->codegen();
        if (!conditionv) {
            return nullptr;
        }

        // Make conditional a bool by comparing it to 0.0
        conditionv = builder->CreateFCmpONE(conditionv, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "ifcond");

        llvm::Function* f = builder->GetInsertBlock()->getParent();

        llvm::BasicBlock* if_block = llvm::BasicBlock::Create(*context, "ifbranch", f);
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(*context, "elsebranch");
        llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(*context, "mergebranch");

        builder->CreateCondBr(conditionv, if_block, else_block);

        builder->SetInsertPoint(if_block);
        llvm::Value* ifv = if_branch->codegen();
        if (!ifv) {
            return nullptr;
        }

        builder->CreateBr(merge_block);
        if_block = builder->GetInsertBlock();

        f->insert(f->end(), else_block);
        builder->SetInsertPoint(else_block);

        llvm::Value* elsev = else_branch->codegen();
        if (!elsev) {
            return nullptr;
        }

        builder->CreateBr(merge_block);
        else_block = builder->GetInsertBlock();

        f->insert(f->end(), merge_block);
        builder->SetInsertPoint(merge_block);
        llvm::PHINode* phi = builder->CreatePHI(llvm::Type::getDoubleTy(*context), 2, "iftmp");
        phi->addIncoming(ifv, if_block);
        phi->addIncoming(elsev, else_block);
        return phi;
    }

    llvm::Value* ForExpression::codegen() {
        llvm::Value* startv = start->codegen();
        if (!startv) {
            return nullptr;
        }

        llvm::Function* f = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* preheader_block = builder->GetInsertBlock();
        llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(*context, "loop", f);

        builder->CreateBr(loop_block);
        builder->SetInsertPoint(loop_block);

        llvm::PHINode* variable = builder->CreatePHI(llvm::Type::getDoubleTy(*context), 2, variable_name);
        variable->addIncoming(startv, preheader_block);

        // Save old variable if the loop variable overrides it
        llvm::Value* old_value = named_values[variable_name];
        named_values[variable_name] = variable;

        if (!body->codegen()) {
            return nullptr;
        }

        llvm::Value* step_value = nullptr;
        if (step) {
            step_value = step->codegen();
            if (!step_value) {
                return nullptr;
            }
        }
        else {
            step_value = llvm::ConstantFP::get(*context, llvm::APFloat(1.0));
        }
        llvm::Value* next_variable = builder->CreateFAdd(variable, step_value, "nextvariable");

        llvm::Value* end_condition = end->codegen();
        if (!end_condition) {
            return nullptr;
        }
        // Convert condition to a bool by comparing to 0.0
        end_condition = builder->CreateFCmpONE(end_condition, llvm::ConstantFP::get(*context, llvm::APFloat(0.0)), "loopcondition");

        //Evaluate if loop should exit
        llvm::BasicBlock* loop_end_block = builder->GetInsertBlock();
        llvm::BasicBlock* after_loop_block = llvm::BasicBlock::Create(*context, "afterloop", f);
        builder->CreateCondBr(end_condition, loop_block, after_loop_block);
        builder->SetInsertPoint(after_loop_block);

        variable->addIncoming(next_variable, loop_end_block);

        // Restore old variable if necessary
        if (old_value) {
            named_values[variable_name] = old_value;
        }
        else {
            named_values.erase(variable_name);
        }

        // return 0
        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*context));
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
            //fpm->run(*f, *fam);
            return f;
        }

        f->eraseFromParent();
        return nullptr;
    }

}
