// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "utils/assert.h"
#include <format>
#include <llvm/IR/Module.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <memory>
#include <string>

namespace kepler {

    enum class OptimizationLevel {
        O0,
        O1,
        O2,
        O3,
        Os,
        Oz,
    };

    llvm::OptimizationLevel get_llvm_optimization_level(OptimizationLevel optimization_level);
    void optimize_module(const std::unique_ptr<llvm::Module>& module, OptimizationLevel optimization_level);

}

template <>
struct std::formatter<kepler::OptimizationLevel> : std::formatter<std::string> {
    auto format(const kepler::OptimizationLevel& optimization_level, std::format_context& ctx) const {
        switch (optimization_level) {
            case kepler::OptimizationLevel::O0:
                return std::formatter<std::string>::format("O0", ctx);
            case kepler::OptimizationLevel::O1:
                return std::formatter<std::string>::format("O1", ctx);
            case kepler::OptimizationLevel::O2:
                return std::formatter<std::string>::format("O2", ctx);
            case kepler::OptimizationLevel::O3:
                return std::formatter<std::string>::format("O3", ctx);
            case kepler::OptimizationLevel::Os:
                return std::formatter<std::string>::format("Os", ctx);
            case kepler::OptimizationLevel::Oz:
                return std::formatter<std::string>::format("Oz", ctx);
        }

        KPL_ASSERT_UNREACHABLE("Missing format implmentation for optimization level '{}'", static_cast<int>(optimization_level));
    }
};
