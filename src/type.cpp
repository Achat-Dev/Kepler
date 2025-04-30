#include <llvm/IR/Type.h>

#include "compiler.hpp"
#include "type.hpp"
#include "utils.hpp"

namespace Kepler {

    llvm::Type* Type::get_by_token(TypeToken type) {
        switch (type) {
            case TypeToken::None: return llvm::Type::getVoidTy(Compiler::get_context());
            case TypeToken::Var: return nullptr;
            case TypeToken::Bool: return nullptr;
            case TypeToken::Char: return nullptr;
            case TypeToken::String: return nullptr;
            case TypeToken::Int8: return nullptr;
            case TypeToken::Int16: return nullptr;
            case TypeToken::Int32: return nullptr;
            case TypeToken::Int64: return nullptr;
            case TypeToken::Float32: return nullptr;
            case TypeToken::Float64: return llvm::Type::getDoubleTy(Compiler::get_context());
            default:
                emergency_exit();
                return nullptr; // Needed to avoid compile warnings
        }
    }

}
