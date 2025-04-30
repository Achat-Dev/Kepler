#include <cmath>
#include <exception>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

#include "compiler.hpp"
#include "log.hpp"
#include "utils.hpp"

namespace Kepler {

    llvm::AllocaInst* create_entry_block_alloca(llvm::Function* f, llvm::StringRef variable_name) {
        llvm::IRBuilder<> tmp_builder(&f->getEntryBlock(), f->getEntryBlock().begin());
        return tmp_builder.CreateAlloca(llvm::Type::getDoubleTy(Compiler::get_context()), nullptr, variable_name);
    }

    void emergency_exit() {
        log(LogStyle::UNSUPPORTED,
            "Roses are red, violets are blue,\n",
            "I reached some code that I never should do.\n",
            "Now here I am, with no helping hand,\n",
            "And a crash and stack trace I don’t understand.");
        std::terminate();
    }

}
