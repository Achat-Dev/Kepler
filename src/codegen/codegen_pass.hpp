// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_pass.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/member_expression.hpp"
#include "ast/expressions/object_initializer_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/function.hpp"
#include "ast/prototype.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "ast/struct.hpp"
#include "codegen/optimizer.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "semantic_analysis/symbol_id.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type.hpp"
#include "type_system/type_table.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct CodegenResult {
        llvm::Value* llvm_value = nullptr;
        bool returns = false;
    };

    class CodegenPass : ASTPass<std::optional<std::vector<std::unique_ptr<llvm::Module>>>> {
    public:
        CodegenPass(DiagnosticSink& diagnostic_sink,
            SymbolTable& symbol_table,
            TypeTable& type_table,
            llvm::LLVMContext& context,
            llvm::TargetMachine* target_machine,
            OptimizationLevel optimization_level);
        std::optional<std::vector<std::unique_ptr<llvm::Module>>> run(std::vector<AbstractSyntaxTree>& ast) override;

    private:
        DiagnosticSink& diagnostic_sink;
        SymbolTable& symbol_table;
        TypeTable& type_table;
        llvm::TargetMachine* target_machine;
        const OptimizationLevel optimization_level;
        llvm::LLVMContext& context;
        llvm::IRBuilder<> builder;
        llvm::Module* current_llvm_module = nullptr;
        std::vector<std::vector<SymbolId>> symbol_id_scopes;
        std::unordered_map<SymbolId, llvm::Value*> llvm_values;
        std::unordered_map<TypeId, llvm::Type*> llvm_types;
        bool main_method_found = false;

        void open_scope();
        void close_scope();
        StringId create_mangled_identifier(StringId identifier_id) const;
        StringId create_mangled_identifier(StringId prototype_identifier_id, const std::vector<TypeId>& parameter_type_ids, bool is_variadic) const;
        bool is_main_method(const Prototype* prototype) const;
        llvm::Type* get_llvm_type(TypeId type_id);

        void forward_declare_structs(const std::vector<Struct*>& struct_nodes);
        void forward_declare_prototypes(const std::vector<ASTNode*>& nodes);
        void forward_declare_prototype(const Prototype* prototype, LinkageKind linkage_kind, bool is_extern);
        void codegen_struct_bodies(const std::vector<Struct*>& struct_nodes);
        void codegen_nodes(const std::vector<ASTNode*>& nodes);
        CodegenResult codegen_node(const ASTNode* node);
        void codegen_function(const Function* function);
        llvm::AllocaInst* create_entry_block_alloca(llvm::Function* function, llvm::Type* type);
        CodegenResult codegen_assignment_statement(const AssignmentStatement* statement);
        CodegenResult codegen_for_statement(const ForStatement* statement);
        CodegenResult codegen_if_statement(const IfStatement* statement);
        CodegenResult codegen_return_statement(const ReturnStatement* statement);
        CodegenResult codegen_variable_definition_statement(const VariableDefinitionStatement* statement);
        CodegenResult codegen_boolean_literal_expression(const BooleanLiteralExpression* expression);
        CodegenResult codegen_floating_point_literal_expression(const FloatingPointLiteralExpression* expression);
        CodegenResult codegen_integer_literal_expression(const IntegerLiteralExpression* expression);
        CodegenResult codegen_string_literal_expression(const StringLiteralExpression* expression);
        CodegenResult codegen_binary_expression(const BinaryExpression* expression);
        CodegenResult codegen_call_expression(const CallExpression* expression);
        CodegenResult create_call(SymbolId prototype_symbol_id,
            const std::vector<Expression*>& arguments,
            const std::string& prototype_identifier);
        CodegenResult codegen_mathematical_negation_expression(const MathematicalNegationExpression* expression);
        CodegenResult codegen_member_expression(const MemberExpression* expression);
        CodegenResult codegen_object_initializer_expression(const ObjectInitializerExpression* expression);
        CodegenResult codegen_variable_expression(const VariableExpression* expression);

        // Operation creation
        llvm::Value* create_add(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_sub(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_mul(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_div(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_less_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_greater_than(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_not_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_less_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_greater_equals(llvm::Value* lhs, llvm::Value* rhs, const Type* type);
        llvm::Value* create_cast(llvm::Value* value, const Type* original_type, const Type* target_type);
        llvm::Value* create_cast_to_bool(llvm::Value* value, const Type* original_type);
        llvm::Value* create_cast_to_f32(llvm::Value* value, const Type* original_type);
        llvm::Value* create_cast_to_f64(llvm::Value* value, const Type* original_type);

        template <typename T>
        llvm::Value* create_cast_float_to_int(llvm::Value* value, const Type* original_type, const Type* target_type) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(target_type);
            KPL_ASSERT_THAT(is_builtin_type(original_type));
            KPL_ASSERT_THAT(is_builtin_type(target_type));
            KPL_ASSERT_THAT(original_type != target_type);
            // Clamp the value of the float to the integer boundaries because llvm poisons the result if the value is out of bounds
            llvm::Type* llvm_float_type = get_builtin_llvm_type(original_type->type_kind, context);
            llvm::Value* min = llvm::ConstantFP::get(llvm_float_type, static_cast<double>(std::numeric_limits<T>::lowest()));
            llvm::Value* max = llvm::ConstantFP::get(llvm_float_type, static_cast<double>(std::numeric_limits<T>::max()));
            llvm::Value* clamped = builder.CreateBinaryIntrinsic(llvm::Intrinsic::maxnum, value, min);
            clamped = builder.CreateBinaryIntrinsic(llvm::Intrinsic::minnum, clamped, max);
            llvm::Type* llvm_target_type = get_builtin_llvm_type(target_type->type_kind, context);
            if (is_signed_integer_type(target_type)) {
                return builder.CreateFPToSI(clamped, llvm_target_type);
            } else if (is_unsigned_integer_type(target_type)) {
                return builder.CreateFPToUI(clamped, llvm_target_type);
            }
            KPL_ASSERT_UNREACHABLE("Target type must be of type int when creating cast from float to int");
        }

        template <typename T>
        llvm::Value* create_cast_to_int(llvm::Value* value, const Type* original_type, const Type* target_type, bool is_signed) {
            KPL_ASSERT_NOT_NULLPTR(value);
            KPL_ASSERT_NOT_NULLPTR(original_type);
            KPL_ASSERT_NOT_NULLPTR(target_type);
            KPL_ASSERT_THAT(is_builtin_type(target_type));
            KPL_ASSERT_THAT(original_type != target_type);
            if (is_integer_type(original_type)) {
                return builder.CreateIntCast(value, get_builtin_llvm_type(target_type->type_kind, context), is_signed);
            } else if (is_floating_point_type(original_type)) {
                return create_cast_float_to_int<T>(value, original_type, target_type);
            }
            KPL_ASSERT_UNREACHABLE("Missing create cast to '{}' implementation for original type '{}'", *target_type, *original_type);
        }
    };

}
