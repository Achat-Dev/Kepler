#include <cmath>
#include <cstdio>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <memory>

#include "compiler.hpp"
#include "optimiser.hpp"

namespace Kepler::Optimiser {

    static std::unique_ptr<llvm::FunctionPassManager> fpm;
    static std::unique_ptr<llvm::FunctionAnalysisManager> fam;
    static std::unique_ptr<llvm::LoopAnalysisManager> lam;
    static std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
    static std::unique_ptr<llvm::ModuleAnalysisManager> mam;
    static std::unique_ptr<llvm::PassInstrumentationCallbacks> pic;
    static std::unique_ptr<llvm::StandardInstrumentations> si;

    const void initialise() {
        fpm = std::make_unique<llvm::FunctionPassManager>();
        fam = std::make_unique<llvm::FunctionAnalysisManager>();
        lam = std::make_unique<llvm::LoopAnalysisManager>();
        cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
        mam = std::make_unique<llvm::ModuleAnalysisManager>();
        pic = std::make_unique<llvm::PassInstrumentationCallbacks>();
        si = std::make_unique<llvm::StandardInstrumentations>(Compiler::Internal::get_context(), true);
        si->registerCallbacks(*pic, mam.get());

        fpm->addPass(llvm::InstCombinePass());
        fpm->addPass(llvm::ReassociatePass());
        fpm->addPass(llvm::GVNPass());
        fpm->addPass(llvm::SimplifyCFGPass());

        // mem2reg passes (crucial for mutable local variables)
        fpm->addPass(llvm::PromotePass());
        fpm->addPass(llvm::InstCombinePass());
        fpm->addPass(llvm::ReassociatePass());

        llvm::PassBuilder passbuilder;
        passbuilder.registerModuleAnalyses(*mam);
        passbuilder.registerFunctionAnalyses(*fam);
        passbuilder.crossRegisterProxies(*lam, *fam, *cgam, *mam);
    }

    const void optimise_function(llvm::Function& f) {
        fpm->run(f, *fam);
    }

}
