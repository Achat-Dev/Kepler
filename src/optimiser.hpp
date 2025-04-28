#pragma once

#include <llvm/IR/Function.h>

namespace Kepler::Optimiser {

    void initialise();
    void optimise_function(llvm::Function& f);

}
