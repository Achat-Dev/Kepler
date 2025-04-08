#pragma once

#include <iostream>
#include <llvm/IR/Value.h>

namespace Kepler {

    template<typename T>
    void log(T t) {
        std::cout << t << std::endl;
    }

    template<typename T, typename... Args>
    void log(T t, Args... args) {
        std::cout << t << ' ';
        log(args...);
    }

}
