// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "codegen/optimizer.hpp"
#include "utils/assert.h"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <memory>

namespace kepler {

    llvm::OptimizationLevel get_llvm_optimization_level(OptimizationLevel optimization_level) {
        switch (optimization_level) {
            case OptimizationLevel::O0:
                return llvm::OptimizationLevel::O0;
            case OptimizationLevel::O1:
                return llvm::OptimizationLevel::O1;
            case OptimizationLevel::O2:
                return llvm::OptimizationLevel::O2;
            case OptimizationLevel::O3:
                return llvm::OptimizationLevel::O3;
            case OptimizationLevel::Os:
                return llvm::OptimizationLevel::Os;
            case OptimizationLevel::Oz:
                return llvm::OptimizationLevel::Oz;
        }
        KPL_ASSERT_UNREACHABLE("Missing llvm mapping implementation for optimization level '{}'", static_cast<int>(optimization_level));
    }

    void optimize_module(const std::unique_ptr<llvm::Module>& module, OptimizationLevel optimization_level) {
        KPL_ASSERT_NOT_NULLPTR(module);
        // The order in which these are created is important for correct destruction
        // This has something to do with inter-analysis-manager references
        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        llvm::PassBuilder pass_builder;
        pass_builder.registerModuleAnalyses(mam);
        pass_builder.registerCGSCCAnalyses(cgam);
        pass_builder.registerFunctionAnalyses(fam);
        pass_builder.registerLoopAnalyses(lam);
        pass_builder.crossRegisterProxies(lam, fam, cgam, mam);

        llvm::ModulePassManager mpm = pass_builder.buildPerModuleDefaultPipeline(get_llvm_optimization_level(optimization_level));
        mpm.run(*module, mam);
    }

}
