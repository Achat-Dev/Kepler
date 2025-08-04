#include "types/type.hpp"

#include "log.hpp"
#include "types/bool_type.hpp"
#include "types/char_type.hpp"
#include "types/data_type.hpp"
#include "types/float32_type.hpp"
#include "types/float64_type.hpp"
#include "types/int16_type.hpp"
#include "types/int32_type.hpp"
#include "types/int64_type.hpp"
#include "types/int8_type.hpp"
#include "types/string_type.hpp"
#include "types/tmap_type.hpp"
#include "types/type_token.hpp"
#include "types/void_type.hpp"

#include <cassert>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <unordered_map>

#define ASSERT_NONE(type, message) assert((type != TypeToken::None) && "[ Assertion ]: trying to " #message " type 'None', which is only used for internal processing and doesn't represent an actual type");
#define ASSERT_TYPE(type) assert((type_map.find(type) != type_map.end()) && "[ Assertion ]: type does not exist in type map");

namespace Kepler::Type {

    static std::unordered_map<TypeToken, std::shared_ptr<DataType>> type_map = {
        { TypeToken::Void, std::make_shared<VoidType>() },
        { TypeToken::TMap, std::make_shared<TMapType>() },
        { TypeToken::Bool, std::make_shared<BoolType>() },
        { TypeToken::Char, std::make_shared<CharType>() },
        { TypeToken::String, std::make_shared<StringType>() },
        { TypeToken::Int8, std::make_shared<Int8Type>() },
        { TypeToken::Int16, std::make_shared<Int16Type>() },
        { TypeToken::Int32, std::make_shared<Int32Type>() },
        { TypeToken::Int64, std::make_shared<Int64Type>() },
        { TypeToken::Float32, std::make_shared<Float32Type>() },
        { TypeToken::Float64, std::make_shared<Float64Type>() },
    };

    bool is_floating_point_type(TypeToken type) {
        return type == TypeToken::Float32 || type == TypeToken::Float64;
    }

    bool is_integer_type(TypeToken type) {
        return type == TypeToken::Int8 || type == TypeToken::Int16 || type == TypeToken::Int32 || type == TypeToken::Int64;
    }

    std::string get_type_name(TypeToken type) {
        ASSERT_NONE(type, "get the name of");
        ASSERT_TYPE(type);
        return type_map[type]->get_name();
    }

    llvm::Type* get_by_token(TypeToken type) {
        ASSERT_NONE(type, "get the llvm type of");
        ASSERT_TYPE(type);
        return type_map[type]->get_llvm_type();
    }

    llvm::Value* cast(llvm::Value* value, TypeToken from, TypeToken to) {
        ASSERT_NONE(from, "cast from");
        ASSERT_NONE(to, "cast to");
        ASSERT_TYPE(from);

        if (from == to) {
            log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": casting a value of type '", from, "' to the same type, which is redundant");
            return value;
        }

        value = type_map[from]->cast(value, to);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": casting from type '", from, "' to type '", to, "' is not supported");
            return nullptr;
        }
        return value;
    }

    bool create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data) {
        ASSERT_NONE(variable_data.type, "create a '=' operation with");
        ASSERT_TYPE(variable_data.type);
        return type_map[variable_data.type]->create_assign(value, variable_data);
    }

    bool create_assign(llvm::Value* value, TypeToken value_type, const LocalVariables::VariableData& variable_data) {
        ASSERT_NONE(variable_data.type, "create a '=' operation with");
        ASSERT_TYPE(variable_data.type);
        return type_map[variable_data.type]->create_assign(value, value_type, variable_data);
    }

    llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '+' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_add(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '+' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '-' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_sub(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '-' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '*' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_mul(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '*' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '/' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_div(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '/' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '<' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_less_than(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '<' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '>' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_greater_than(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '>' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '==' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_equals(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '==' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '!=' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_not_equals(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '!=' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '<=' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_less_equals(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '<=' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

    llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs, TypeToken type) {
        ASSERT_NONE(type, "create a '>=' operation with");
        ASSERT_TYPE(type);
        llvm::Value* value = type_map[type]->create_greater_equals(lhs, rhs);
        if (value == nullptr) {
            log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": '>=' operation between type '", type, "' is not supported");
            return nullptr;
        }
        return value;
    }

}
