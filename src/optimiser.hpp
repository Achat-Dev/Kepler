#pragma once

#include <llvm/IR/Function.h>

namespace Kepler::Optimiser {

    const void initialise();
    const void optimise_function(llvm::Function& f);

}
