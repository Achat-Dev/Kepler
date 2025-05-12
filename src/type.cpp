#include <exception>
#include <llvm/IR/Type.h>
#include <string>

#include "compiler.hpp"
#include "type.hpp"
#include "log.hpp"
#include "utils.hpp"

namespace Kepler::Type {

    static std::string to_string(const TypeToken& type) {
        switch (type) {
            case TypeToken::None: return "none";
            case TypeToken::Var: return "var";
            case TypeToken::Bool: return "bool";
            case TypeToken::Char: return "char";
            case TypeToken::String: return "String";
            case TypeToken::Int8: return "i8";
            case TypeToken::Int16: return "i16";
            case TypeToken::Int32: return "i32";
            case TypeToken::Int64: return "i64";
            case TypeToken::Float32: return  "f32";
            case TypeToken::Float64: return "f64";
        }

        return "unknown type";
    }

    std::ostream& operator<<(std::ostream& os, const TypeToken& type) {
        os << to_string(type);
        return os;
    }

    llvm::Type* get_by_token(TypeToken type) {
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
            case TypeToken::Float32: return llvm::Type::getFloatTy(Compiler::get_context());
            case TypeToken::Float64: return llvm::Type::getDoubleTy(Compiler::get_context());
            default:
                emergency_exit("type '" + to_string(type) + "' does not map to an llvm type");
                return nullptr; // Needed to avoid compile warnings
        }
    }

}
