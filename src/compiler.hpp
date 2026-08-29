// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "codegen/optimizer.hpp"
#include "diagnostics/diagnostic.hpp"
#include <expected>
#include <filesystem>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <string>
#include <vector>

namespace kepler {

    struct CompilerContext {
        std::filesystem::path input_path;
        std::filesystem::path output_path;
        std::vector<std::filesystem::path> additional_paths;
        OptimizationLevel optimization_level = OptimizationLevel::O2;
        bool version_requested = false;
        bool help_requested = false;
        std::string help;
    };

    class Compiler {
    public:
        int run(int argc, char** argv) const;

    private:
        std::expected<CompilerContext, Diagnostic> parse_args(int argc, char** argv) const;
        void verify_ast(const AbstractSyntaxTree& ast) const;
        llvm::TargetMachine* create_target_machine() const;
        bool emit_object_code(const std::unique_ptr<llvm::Module>& module,
            llvm::TargetMachine* target_machine,
            const std::filesystem::path& output_path) const;
        bool link_to_executable(const std::filesystem::path& object_path,
            const std::vector<std::filesystem::path>& additional_paths,
            OptimizationLevel optimization_level,
            const std::filesystem::path& output_path) const;
    };

}
