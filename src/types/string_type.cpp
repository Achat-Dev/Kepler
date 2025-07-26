#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "string_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* StringType::get_llvm_type() const {
        // A string is internally represented as an immutable array of i8
        // However, to get the llvm::Type* of that, the length of the array is needed
        // That's why the type of a string is an i8* (since llvm uses opaque pointers, the pointer is not explicitly typed)
        return llvm::PointerType::get(Compiler::get_context(), 0);
    }

    llvm::Value* StringType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from type 'string' is not supported yet");
        return nullptr;
    }

    std::string StringType::get_name() const {
        return "string";
    }

}
