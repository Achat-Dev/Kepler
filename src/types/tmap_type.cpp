#include "types/tmap_type.hpp"

#include "compiler.hpp"
#include "log.hpp"
#include "types/type.hpp"
#include "types/type_token.hpp"
#include "utils.hpp"

#include <cassert>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

namespace Kepler::Type {

    // Layout the members for the least amount of padding -> sort from most memory to least memory
    static TypeToken tmap_member_order[] = {
        TypeToken::Int64,
        TypeToken::Float64,
        TypeToken::String,
        TypeToken::Int32,
        TypeToken::Float32,
        TypeToken::Int16,
        TypeToken::Int8,
        //TypeToken::Char,
        TypeToken::Bool,
    };
    static const int field_count = sizeof(tmap_member_order) / sizeof(tmap_member_order[0]);

    static llvm::StructType* llvm_type = nullptr;

    void TMapType::create_type() {
        llvm::StructType* struct_type = llvm::StructType::create(Compiler::get_context(), "tmap_type");

        // Create the struct body
        llvm::Type* struct_body[field_count * 2];
        for (size_t i = 0; i < field_count; i++) {
            struct_body[i] = get_by_token(tmap_member_order[i]);
        }

        // Push the flags if the fields have been set;
        llvm::Type* flag_type = get_by_token(TypeToken::Bool);
        for (size_t i = field_count; i < field_count * 2; i++) {
            struct_body[i] = flag_type;
        }
        struct_type->setBody(struct_body);

        llvm_type = struct_type;
    }

    std::string TMapType::get_name() const {
        return "tmap";
    }

    llvm::Type* TMapType::get_llvm_type() const {
        return llvm_type;
    }

    llvm::Value* TMapType::cast(llvm::Value* value, TypeToken to) const {
        for (size_t i = 0; i < field_count; i++) {
            if (tmap_member_order[i] == to) {
                // Get alloca
                llvm::LoadInst* loadinst = llvm::dyn_cast<llvm::LoadInst>(value);
                if (!loadinst) {
                    log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": failed to read field of type '", to, "' from a 'tmap' because the 'llvm::Value*' is not an 'llvm::LoadInst*'. You are probably trying to cast the return value of a function directly, try to assign it to a variable first.");
                    return nullptr;
                }
                llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(loadinst->getPointerOperand());
                if (!alloca) {
                    log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": failed to read field of type '", to, "' from a 'tmap' because the 'llvm::LoadInst*' is not an 'llvm::AllocaInst*'. You are probably trying to cast the return value of a function directly, try to assign it to a variable first.");
                    return nullptr;
                }

                // Check if a value has been assigned to the field
                llvm::Value* field_flag_ptr = Compiler::get_builder().CreateStructGEP(llvm_type, alloca, i + field_count, "tmap_field_flag_ptr");
                llvm::Value* field_flag_value = Compiler::get_builder().CreateLoad(get_by_token(TypeToken::Bool), field_flag_ptr, "tmap_field_flag");
                llvm::Value* condition = Type::create_equals(field_flag_value, llvm::ConstantInt::getFalse(Compiler::get_context()), TypeToken::Bool);

                llvm::Function* f = Compiler::get_builder().GetInsertBlock()->getParent();
                llvm::BasicBlock* if_block = llvm::BasicBlock::Create(Compiler::get_context(), "ifbranch", f);
                llvm::BasicBlock* else_block = llvm::BasicBlock::Create(Compiler::get_context(), "elsebranch", f);
                Compiler::get_builder().CreateCondBr(condition, if_block, else_block);
                Compiler::get_builder().SetInsertPoint(if_block);

                throw_runtime_error("trying to read field of type '" + get_type_name(to) + "' from a 'tmap', which hasn't been assigned yet");
                Compiler::get_builder().CreateUnreachable();

                // Load the actual field
                Compiler::get_builder().SetInsertPoint(else_block);
                llvm::Value* field_ptr = Compiler::get_builder().CreateStructGEP(llvm_type, alloca, i, "tmap_field_ptr");
                llvm::Value* field_value = Compiler::get_builder().CreateLoad(get_by_token(to), field_ptr, "tmap_field");
                return field_value;
            }
        }
        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": trying to read type '", to, "' from a 'tmap', which doesn't exist in a 'tmap'");
        return nullptr;
    }

    bool TMapType::create_assign(llvm::Value* value, const LocalVariables::VariableData& variable_data) const {
        emergency_exit("trying to assign a value to a variable of type 'tmap' without specifying the type of the value");
        return false;
    }

    bool TMapType::create_assign(llvm::Value* value, TypeToken value_type, const LocalVariables::VariableData& variable_data) const {
        if (value_type == TypeToken::TMap) {
            Compiler::get_builder().CreateStore(value, variable_data.variable);
            return true;
        }
        else {
            for (size_t i = 0; i < field_count; i++) {
                if (tmap_member_order[i] == value_type) {
                    // Assign the value to the field
                    llvm::Value* field_ptr = Compiler::get_builder().CreateStructGEP(llvm_type, variable_data.variable, i, "tmap_field_ptr");
                    Compiler::get_builder().CreateStore(value, field_ptr);

                    // Set the flag to indicate that the field has a value assigned to it
                    llvm::Value* field_flag_ptr = Compiler::get_builder().CreateStructGEP(llvm_type, variable_data.variable, i + field_count, "tmap_field_flag_ptr");
                    Compiler::get_builder().CreateStore(llvm::ConstantInt::getTrue(Compiler::get_context()), field_flag_ptr);
                    return true;
                }
            }
        }

        log(LogStyle::ERROR, "[ Compile error ]", LogStyle::DEFAULT, ": trying to assign a value of type '", value_type, "' to a variable of type 'tmap', which doesn't exist in a 'tmap'");
        return false;
    }

}
