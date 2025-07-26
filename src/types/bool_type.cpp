#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "bool_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"

namespace Kepler::Type {

    llvm::Type* BoolType::get_llvm_type() const {
        return llvm::Type::getInt1Ty(Compiler::get_context());
    }

    llvm::Value* BoolType::cast(llvm::Value* value, TypeToken to) const {
        switch (to) {
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'bool' to 'string' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    std::string BoolType::get_name() const {
        return "bool";
    }

}
