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
#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include <expected>
#include <filesystem>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kepler {

    struct CmdArgs {
        std::vector<std::filesystem::path> input_paths;
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
        // TODO (improvement): Some of these functions have a lot of long arguments
        std::expected<void, Diagnostic> do_dependencies_exist() const;
        std::expected<CmdArgs, Diagnostic> parse_args(int argc, char** argv) const;

        std::optional<std::vector<AbstractSyntaxTree>> create_asts(const std::vector<std::filesystem::path> file_paths,
            DiagnosticSink& diagnostic_sink,
            TypeTable& type_table) const;
        void verify_ast(AbstractSyntaxTree& ast, const File* file) const;

        std::optional<std::vector<std::unique_ptr<llvm::Module>>> run_ast_passes(std::vector<AbstractSyntaxTree>& asts,
            DiagnosticSink& diagnostic_sink,
            SymbolTable& symbol_table,
            TypeTable& type_table,
            llvm::LLVMContext& llvm_context,
            llvm::TargetMachine* target_machine,
            OptimizationLevel optimization_level) const;
        llvm::TargetMachine* create_target_machine() const;

        bool create_executable(std::vector<std::unique_ptr<llvm::Module>>& llvm_modules,
            llvm::TargetMachine* target_machine,
            const std::vector<std::filesystem::path>& additional_paths,
            OptimizationLevel optimization_level,
            const std::filesystem::path& output_path) const;
        bool emit_object_code(const std::unique_ptr<llvm::Module>& module,
            llvm::TargetMachine* target_machine,
            const std::filesystem::path& output_path) const;
        bool link_to_executable(const std::vector<std::filesystem::path>& object_paths,
            const std::vector<std::filesystem::path>& additional_paths,
            OptimizationLevel optimization_level,
            const std::filesystem::path& output_path) const;

        void print_diagnostic(const Diagnostic& diagnostic) const;
    };

}
