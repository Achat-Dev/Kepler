#include <exception>
#include <llvm/IR/Type.h>

#include "compiler.hpp"
#include "type.hpp"
#include "log.hpp"
#include "utils.hpp"

namespace Kepler {

    llvm::Type* Type::get_by_token(TypeToken type) {
        switch (type) {
            case TypeToken::None: return llvm::Type::getVoidTy(Compiler::get_context());
            case TypeToken::Var:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'var' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Bool:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'bool' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'char' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'string' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'i8' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'i16' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'i32' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'i64' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'f32' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Float64: return llvm::Type::getDoubleTy(Compiler::get_context());
            default:
                emergency_exit();
                return nullptr; // Needed to avoid compile warnings
        }
    }

}
