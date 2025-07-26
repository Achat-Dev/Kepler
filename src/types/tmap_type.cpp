#include <exception>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"
#include "tmap_type.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* TMapType::get_llvm_type() const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'tmap' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* TMapType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": type 'tmap' cannot be casted");
        return nullptr;
    }

    std::string TMapType::get_name() const {
        return "tmap";
    }

}
