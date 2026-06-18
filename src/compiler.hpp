// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "file.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>

namespace Kepler {

    class Compiler {
    public:
        bool compile_file();
        llvm::LLVMContext& get_context() const { return *context; };
        llvm::IRBuilder<>& get_builder() const { return *builder; };
        llvm::Module& get_module() const { return *module; };
        std::unique_ptr<File>& get_file() { return file; };

        static Compiler& get();

    private:
        Compiler() = default;
        bool initialise();
        bool write_object_file(const std::string& output_name);
        bool compile_executable(const std::string& outname);

        std::unique_ptr<llvm::LLVMContext> context;
        std::unique_ptr<llvm::IRBuilder<>> builder;
        std::unique_ptr<llvm::Module> module;
        std::unique_ptr<File> file;

        llvm::TargetMachine* target_machine;
    };

}

/*namespace Kepler::Compiler {

    llvm::LLVMContext& get_context();
    llvm::IRBuilder<>& get_builder();
    llvm::Module& get_module();
    std::unique_ptr<File>& get_file();

    bool compile_file();

}*/
