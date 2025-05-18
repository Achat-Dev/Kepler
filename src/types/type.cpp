#include <cassert>
#include <exception>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"
#include "../utils.hpp"

namespace Kepler::Type {

    std::ostream& operator<<(std::ostream& os, TypeToken type) {
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
            case TypeToken::Int8: return llvm::Type::getInt8Ty(Compiler::get_context());
            case TypeToken::Int16: return llvm::Type::getInt16Ty(Compiler::get_context());
            case TypeToken::Int32: return llvm::Type::getInt32Ty(Compiler::get_context());
            case TypeToken::Int64: return llvm::Type::getInt64Ty(Compiler::get_context());
            case TypeToken::Float32: return llvm::Type::getFloatTy(Compiler::get_context());
            case TypeToken::Float64: return llvm::Type::getDoubleTy(Compiler::get_context());
            default:
                emergency_exit("type '" + to_string(type) + "' does not map to an llvm type");
                return nullptr; // Needed to avoid compile warnings
        }
    }

    static llvm::Value* cast_i8(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int8), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'i16' is not supported yet");
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'i32' is not supported yet");
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'i64' is not supported yet");
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'f32' is not supported yet");
                return nullptr;
            case TypeToken::Float64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i8' to 'f64' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    static llvm::Value* cast_i16(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int16), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'i8' is not supported yet");
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'i32' is not supported yet");
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'i64' is not supported yet");
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'f32' is not supported yet");
                return nullptr;
            case TypeToken::Float64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i16' to 'f64' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    static llvm::Value* cast_i32(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int32), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'i8' is not supported yet");
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'i16' is not supported yet");
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'i64' is not supported yet");
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'f32' is not supported yet");
                return nullptr;
            case TypeToken::Float64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i32' to 'f64' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    static llvm::Value* cast_i64(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateICmpNE(value, llvm::ConstantInt::get(get_by_token(TypeToken::Int64), 0));
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'i8' is not supported yet");
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'i16' is not supported yet");
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'i32' is not supported yet");
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'f32' is not supported yet");
                return nullptr;
            case TypeToken::Float64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'i64' to 'f64' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    static llvm::Value* cast_f32(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float32), 0));
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'i8' is not supported yet");
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'i16' is not supported yet");
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'i32' is not supported yet");
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'i64' is not supported yet");
                return nullptr;
            case TypeToken::Float64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f32' to 'f64' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    static llvm::Value* cast_f64(llvm::Value* value, TypeToken to) {
        switch (to) {
            case TypeToken::Bool:
                return Compiler::get_builder().CreateFCmpONE(value, llvm::ConstantFP::get(get_by_token(TypeToken::Float64), 0));
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'i8' is not supported yet");
                return nullptr;
            case TypeToken::Int16:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'i16' is not supported yet");
                return nullptr;
            case TypeToken::Int32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'i32' is not supported yet");
                return nullptr;
            case TypeToken::Int64:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'i64' is not supported yet");
                return nullptr;
            case TypeToken::Float32:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": casting from 'f64' to 'f32' is not supported yet");
                return nullptr;
            default: return nullptr;
        }
    }

    llvm::Value* cast(llvm::Value* value, TypeToken from, TypeToken to) {

        assert((from != Type::TypeToken::None && from != Type::TypeToken::Var) && "[ Assertion ]: trying to cast from an invalid type");
        assert((to != Type::TypeToken::None && to != Type::TypeToken::Var) && "[ Assertion ]: trying to cast to an invalid type");

        if (from == to) {
            log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": casting a value of type '", from, "' to the same type, which is redundant");
            return value;
        }

        switch (from) {
            case TypeToken::Bool:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'bool' is not supported yet");
                return nullptr;
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'string' is not supported yet");
                return nullptr;
            case TypeToken::Int8:
                if ((value = cast_i8(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int16:
                if ((value = cast_i16(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int32:
                if ((value = cast_i32(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int64:
                if ((value = cast_i64(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Float32:
                if ((value = cast_f32(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Float64:
                if ((value = cast_f64(value, to))) {
                    return value;
                }
                break;
            default: break;
        }

        // Don't make this a default because of the nested switch
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": casting from type '", from, "' to type '", to, "' is not supported");
        return nullptr;
    }

    std::string to_string(TypeToken type) {
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

    bool is_floating_point_type(TypeToken type) {
        return type == TypeToken::Float32 || type == TypeToken::Float64;
    }

    bool is_integer_type(TypeToken type) {
        return type == TypeToken::Int8 || type == TypeToken::Int16 || type == TypeToken::Int32 || type == TypeToken::Int64;
    }

}
