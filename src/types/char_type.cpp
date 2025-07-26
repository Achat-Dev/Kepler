#include <exception>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "char_type.hpp"
#include "type.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* CharType::get_llvm_type() const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    std::string CharType::get_name() const {
        return "char";
    }

}
