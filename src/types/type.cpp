#include <cassert>
#include <exception>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

#include "bool_type.hpp"
#include "float32_type.hpp"
#include "float64_type.hpp"
#include "int16_type.hpp"
#include "int32_type.hpp"
#include "int64_type.hpp"
#include "int8_type.hpp"
#include "string_type.hpp"
#include "type.hpp"
#include "../compiler.hpp"
#include "../log.hpp"
#include "../utils.hpp"

namespace Kepler::Type {

    std::ostream& operator<<(std::ostream& os, TypeToken type) {
        os << get_type_name(type);
        return os;
    }

    llvm::Type* get_by_token(TypeToken type) {
        switch (type) {
            case TypeToken::Void: return llvm::Type::getVoidTy(Compiler::get_context());
            case TypeToken::TMap:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'var' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::Bool: return BoolType::get_llvm_type();
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'char' is not supported yet");
                std::terminate();
                return nullptr;
            case TypeToken::String: return StringType::get_llvm_type();
            case TypeToken::Int8: return Int8Type::get_llvm_type();
            case TypeToken::Int16: return Int16Type::get_llvm_type();
            case TypeToken::Int32: return Int32Type::get_llvm_type();
            case TypeToken::Int64: return Int64Type::get_llvm_type();
            case TypeToken::Float32: return Float32Type::get_llvm_type();
            case TypeToken::Float64: return Float64Type::get_llvm_type();
            default:
                emergency_exit("type '" + get_type_name(type) + "' does not map to an llvm type");
                return nullptr; // Needed to avoid compile warnings
        }
    }

    llvm::Value* cast(llvm::Value* value, TypeToken from, TypeToken to) {
        assert((from != Type::TypeToken::None && from != Type::TypeToken::TMap) && "[ Assertion ]: trying to cast from an invalid type");
        assert((to != Type::TypeToken::None && to != Type::TypeToken::TMap) && "[ Assertion ]: trying to cast to an invalid type");

        if (from == to) {
            log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": casting a value of type '", from, "' to the same type, which is redundant");
            return value;
        }

        switch (from) {
            case TypeToken::Bool:
                if ((value = BoolType::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Char:
                log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": data type 'char' is not supported yet");
                return nullptr;
            case TypeToken::String:
                if ((value = StringType::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int8:
                if ((value = Int8Type::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int16:
                if ((value = Int16Type::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int32:
                if ((value = Int32Type::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Int64:
                if ((value = Int64Type::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Float32:
                if ((value = Float32Type::cast(value, to))) {
                    return value;
                }
                break;
            case TypeToken::Float64:
                if ((value = Float64Type::cast(value, to))) {
                    return value;
                }
                break;
            default: break;
        }

        // Don't make this a default because of the nested switch
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": casting from type '", from, "' to type '", to, "' is not supported");
        return nullptr;
    }

    std::string get_type_name(TypeToken type) {
        switch (type) {
            case TypeToken::None: return "none";
            case TypeToken::Void: return "void";
            case TypeToken::TMap: return "tmap";
            case TypeToken::Bool: return BoolType::get_name();
            case TypeToken::Char: return "char";
            case TypeToken::String: return StringType::get_name();
            case TypeToken::Int8: return Int8Type::get_name();
            case TypeToken::Int16: return Int16Type::get_name();
            case TypeToken::Int32: return Int32Type::get_name();
            case TypeToken::Int64: return Int64Type::get_name();
            case TypeToken::Float32: return Float32Type::get_name();
            case TypeToken::Float64: return Float64Type::get_name();
            default: return "unknown type";
        }
    }

    bool is_floating_point_type(TypeToken type) {
        return type == TypeToken::Float32 || type == TypeToken::Float64;
    }

    bool is_integer_type(TypeToken type) {
        return type == TypeToken::Int8 || type == TypeToken::Int16 || type == TypeToken::Int32 || type == TypeToken::Int64;
    }

}
