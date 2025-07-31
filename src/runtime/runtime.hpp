#pragma once

#include <llvm/IR/Module.h>
#include <memory>

namespace Kepler::Runtime {

    std::unique_ptr<llvm::Module> create();

}
